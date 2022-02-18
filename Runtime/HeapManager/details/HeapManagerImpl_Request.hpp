#pragma once

#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
template <
    typename ResourceBuilderT,
    typename BuildParamTupleFunc
>
struct ResourceRequestHandler
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ResourceT = typename ResourceBuilderT::ProductT;

    HeapManager *manager = nullptr;

    const ResourceBuildOptions *options = nullptr;
    TaskExecutor *executor = nullptr;
    const BuildParamTupleFunc &param_func;

    ResourceRequestHandler(
        HeapManager *manager,
        const ResourceBuildOptions *options,
        TaskExecutor *executor,
        const BuildParamTupleFunc &param_func)
        : manager(manager)
        , options(options)
        , executor(executor)
        , param_func(param_func)
    {
    }

    HeapResourceDescriptor descriptor;
    TargetHeapT *heap = nullptr;
    std::optional<decltype(param_func())> param_tuple;
    std::unique_lock<std::mutex> lock;
    std::unique_ptr<ResourceBuildTask<ResourceBuilderT>> task;
    std::promise<void> promise;
    ResourceAccessor<ResourceBuilderT> accessor;
    ResourceState state;

    void build_descriptor()
    {
        assert(!param_tuple.has_value());

        // Result from std::forward_as_tuple()/make_tuple()
        // Be sure that the parameters returned by the tuple refer to valid
        // memory addresses.
        param_tuple.emplace(param_func());

        // Hash the builder type & parameters to get the id of the resource.
        // Building the hash won't alter the content of the tuple.
        descriptor = details::heap_manager::make_resource_descriptor_from_tuple<
            ResourceBuilderT
        >(std::forward<decltype(param_func())>(param_tuple.value()));
    }

    void ensure_descriptor()
    {
        // If the resource id is unknown, we must construct the builder to
        // obtain it.
        if(!options->requested_resource)
        {
            build_descriptor();
        }
        else
        {
            descriptor = options->requested_resource;
            validate_builder_type();
        }
    }

    void validate_builder_type() const
    {
        // Validate builder type.
        USAGI_ASSERT_THROW(
            descriptor.heap_id() == typeid(TargetHeapT).hash_code(),
            std::runtime_error("Builder type doesn't match with the one "
                "used to create the resource.")
        );
    }

    // Return an accessor to the specified resource or a proper fallback.
    auto make_accessor(HeapResourceDescriptor desc, bool is_fallback)
    {
        return manager->make_accessor_nolock<ResourceBuilderT>(
            desc,
            heap,
            is_fallback
        );
    }

    void get_heap()
    {
        // Try to get the heap. The heap must exist before the resource
        // could be fetched or built. If this fails, exception will be thrown.
        heap = manager->locate_heap<TargetHeapT>();
    }

    void init_accessor()
    {
        // ==================== Enter Critical Section ====================== //
        // -------------------------------------------------------------------//
        // .................................................................. //
        //
        // Read/Write the resource entry table.
        // Resource entry is also used to track the ref count of the resources.
        //

        // Lock the table.
        std::unique_lock lk { manager->mEntryMapMutex };
        lock.swap(lk);

        accessor = make_accessor(descriptor, false);
        state = accessor.last_state();
    }

    template <bool Immediate>
    auto & build_resource()
    {
        // When turned into preparing state, it's guaranteed that no another
        // build task will be created.
        accessor.mEntry->state.store(
            ResourceState::PREPARING,
            std::memory_order::release
        );

        // Create the promise outside the construction of the builder task
        // to shorten the critical section.
        accessor.mEntry->future = promise.get_future();

        // The future object is accessible from now on and the resource entry
        // enters a state that the entry will not be recreated/removed.
        // Safe to unlock here. Our accessor holds a ref to it so it won't be
        // removed.
        lock.unlock();

        // .................................................................. //
        // ------------------------------------------------------------------ //
        // ===================== Exit Critical Section ====================== //

        LOG(trace,
            "[Heap] Building resource: {} (builder={}, resource={})",
            descriptor,
            typeid(ResourceBuilderT).name(),
            typeid(typename ResourceBuilderT::ProductT).name()
        );

        // Ensure parameters are evaluated.
        if constexpr(!Immediate)
        {
            if(!param_tuple) param_tuple.emplace(param_func());
        }
        else
        {
            assert(param_tuple && "Should have been initialized when building "
                "the descriptor.");
        }
            
        // Create build task.
        task = std::apply([&]<typename... Args>(Args &&...args)
        {
            // todo pool the task objects
            return std::make_unique<ResourceBuildTask<ResourceBuilderT>>(
                manager,
                executor, // todo: executor injected in run_build_task_synced
                accessor,
                std::move(promise),
                std::forward<Args>(args)...
            );
        }, param_tuple.value());

        accessor.mEntry->state.store(
            ResourceState::SCHEDULED,
            std::memory_order::release
        );

        // If no executor is provided, build the resource on the current thread.
        if constexpr(Immediate)
        {
            details::heap_manager::run_build_task_synced(std::move(task));
        }
        else
        {
            // Submit the task to the executor.
            details::heap_manager::submit_build_task(executor, std::move(task));

            // If the user wants to use a fallback, return the fallback.
            if(options->fallback_when_building)
            {
                // Creating accessor requires locking.
                lock.lock();
                accessor = make_accessor(options->fallback_when_building, true);
            }
        }

        accessor.fetch_state();

        if constexpr(Immediate)
        {
            // Validate asset state. These are the only two states that
            // should appear after the build task is finished.
            assert(accessor.last_state().failed() ||
                accessor.last_state().ready());
        }

        return accessor;
    }

    auto branch_on_resource_state()
    {
        // Ready state won't change when in critical section.
        if(state.ready()) [[likely]]
        {
            return accessor;
        }
        // If this is the first request, build the resource.
        // ABSENT state won't change either. Only critical sections can alter it.
        if(state == ResourceState::ABSENT_FIRST_REQUEST)
        {
            return build_resource<false>();
        }
        // If the resource has been evicted, see whether to rebuild or use
        // fallback.
        if(state == ResourceState::ABSENT_EVICTED)
        {
            if(options->rebuild_if_evicted)
                return build_resource<false>();
            if(options->fallback_if_evicted)
                return make_accessor(options->fallback_if_evicted, true);
        }
        // Failed state will be only changed here too.
        else if(state.failed()) [[unlikely]]
        {
            if(options->rebuild_if_failed)
                return build_resource<false>();
            if(options->fallback_if_failed)
                return make_accessor(options->fallback_if_failed, true);
        }
        // Otherwise, either the resource is being built, or the user doesn't
        // want to use a fallback. The state of the resource could be volatile.
        // The state that we just fetched will be returned to the user.
        return accessor;
    }

    auto process_request()
    {
        assert(executor);
        assert(options);

        ensure_descriptor();
        get_heap();
        init_accessor();

        return branch_on_resource_state();
    }

    auto process_request_immediate()
    {
        assert(!executor);
        assert(!options);

        build_descriptor();
        get_heap();
        init_accessor();

        // Manually simplified code path.
        if(state == ResourceState::ABSENT_FIRST_REQUEST)
        {
            return build_resource<true>();
        }
        return accessor;
    }
};

template <ResourceBuilder ResourceBuilderT, typename BuildParamTupleFuncT>
auto HeapManager::resource(
    HeapResourceDescriptor resource_cache_id,
    TaskExecutor *executor,
    BuildParamTupleFuncT &&lazy_build_params)
-> ResourceRequestBuilder<ResourceBuilderT, BuildParamTupleFuncT>
requires ConstructibleFromTuple<
    ResourceBuilderT,
    decltype(lazy_build_params())
// The build params shouldn't return rvalue refs like integer literals.
// Because at the point of using, they are much likely already out-of-scope.
// todo: this however won't prevent forwarding refs to local variables.
> && NoRvalueRefInTuple<decltype(lazy_build_params())>
{
    // The request really happens when RequestBuilder.make_request()
    // is called.
    return {
        this,
        executor,
        resource_cache_id,
        std::forward<BuildParamTupleFuncT>(lazy_build_params)
    };
}

/**
 * \brief
 * \tparam ResourceBuilderT
 * \tparam BuildArgs
 * \param args `ResourceBuilderT(args)`
 * \return
 */
template <
    ResourceBuilder ResourceBuilderT,
    typename... BuildArgs
>
auto HeapManager::resource_transient(BuildArgs &&...args)
-> ResourceAccessor<ResourceBuilderT>
requires std::constructible_from<ResourceBuilderT, BuildArgs...>                
{
	auto params = [&] {
        return std::forward_as_tuple(std::forward<BuildArgs>(args)...);
    };

    ResourceRequestHandler<ResourceBuilderT, decltype(params)> handler {
        this,
        nullptr,
        nullptr,
        params
    };

    return handler.process_request_immediate();
}

template <
    ResourceBuilder ResourceBuilderT,
    typename BuildParamTupleFunc,
    bool ImmediateResource
>
constexpr auto HeapManager::request_resource(
    const ResourceBuildOptions *options,
    TaskExecutor *executor,
    BuildParamTupleFunc &&param_func)
-> ResourceAccessor<ResourceBuilderT>
{
    ResourceRequestHandler<ResourceBuilderT, BuildParamTupleFunc> handler {
        this,
        options,
        executor,
        std::forward<BuildParamTupleFunc>(param_func)
    };

    return handler.process_request();
}
}
