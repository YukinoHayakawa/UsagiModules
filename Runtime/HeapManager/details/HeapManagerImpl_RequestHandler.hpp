#pragma once

#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
template <typename Builder, typename LazyBuildArgFunc>
struct ResourceRequestHandler
{
    using TargetHeapT = typename Builder::TargetHeapT;
    using ResourceT = typename Builder::ProductT;
    using ContextT = UniqueResourceRequestContext<Builder, LazyBuildArgFunc>;

    ContextT context;

    ResourceBuildOptions & options() const
    {
        return context->options;
    }

    HeapManager * manager() const
    {
        return context->manager;
    }

    explicit ResourceRequestHandler(ContextT context)
        : context(std::move(context))
    {
    }

    // todo constructor too many var inits adds overhead

    std::optional<decltype(std::declval<LazyBuildArgFunc>()())> param_tuple;
    std::unique_lock<std::mutex> entry_map_lock;
    ResourceAccessor<Builder> accessor;

    void ensure_build_args()
    {
        if(!param_tuple)
            param_tuple.emplace((*context->arg_func)());
    }

    void ensure_requested_descriptor()
    {
        if(options().requested_resource)
            return;

        // Result from std::forward_as_tuple()/make_tuple()
        // Be sure that the parameters returned by the tuple refer to valid
        // memory addresses.
        ensure_build_args();

        // Hash the builder type & parameters to get the id of the resource.
        // Building the hash won't alter the content of the tuple.
        options().requested_resource = 
            details::heap_manager::make_resource_descriptor_from_tuple<
            Builder
        >(param_tuple.value());
    }

    void validate_builder_type() const
    {
        const auto left = options().requested_resource.builder_id();
        const auto right = typeid(Builder).hash_code();

        // Validate builder type.
        USAGI_ASSERT_THROW(
            left == right,
            std::runtime_error("Builder type doesn't match with the one "
                "used to create the resource.")
        );
    }

    // Return an accessor to the specified resource or a proper fallback.
    auto make_accessor(HeapResourceDescriptor desc, bool is_fallback)
    {
        return context->manager->template make_accessor_nolock<Builder>(
            desc,
            context->heap,
            is_fallback
        );
    }

    void get_heap()
    {
        // Try to get the heap. The heap must exist before the resource
        // could be fetched or built. If this fails, exception will be thrown.
        context->heap = context->manager->template locate_heap<TargetHeapT>();
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
        std::unique_lock lk { context->manager->mEntryMapMutex };
        entry_map_lock.swap(lk);

        accessor = make_accessor(options().requested_resource, false);
        context->entry = accessor.mEntry;
    }

    template <bool Transient>
    auto & build_resource()
    {
        // When turned into preparing state, it's guaranteed that no another
        // build task will be created.
        context->entry->state.store(
            ResourceState::PREPARING,
            std::memory_order::release
        );

        // Create the promise outside the construction of the builder task
        // to shorten the critical section.
        // todo transient resource doesn't need a promise.
        std::promise<void> promise;
        context->entry->future = promise.get_future();

        // The future object is accessible from now on and the resource entry
        // enters a state that the entry will not be recreated/removed.
        // Safe to unlock here. Our accessor holds a ref to it so it won't be
        // removed.
        entry_map_lock.unlock();

        // .................................................................. //
        // ------------------------------------------------------------------ //
        // ===================== Exit Critical Section ====================== //

        LOG(trace,
            "[Heap] Building resource: {} (builder={}, resource={})",
            context->entry->descriptor,
            typeid(Builder).name(),
            typeid(typename Builder::ProductT).name()
        );

        // Ensure parameters are evaluated.
        if constexpr(!Transient)
        {
            ensure_build_args();
        }
        else
        {
            assert(param_tuple && "Should have been initialized when building "
                "the descriptor.");
        }

        // Build context ownership is being transferred to build task.
        const auto weak_context = context.get();

        // Create build task.
        // todo eval build args into the builder directly
        std::unique_ptr<ResourceBuildTask<Builder>> task;
        task = std::apply([&]<typename... Args>(Args &&...args)
        {
            return std::make_unique<ResourceBuildTask<Builder>>(
                std::move(context),
                std::move(promise),
                std::forward<Args>(args)...
            );
            // todo pool the task objects
            // todo: fix executor injected in run_build_task_synced
        }, param_tuple.value());

        // context transferred! don't use it anymore.
        
        weak_context->entry->state.store(
            ResourceState::SCHEDULED,
            std::memory_order::release
        );

        // If no executor is provided, build the resource on the current thread.
        if constexpr(Transient)
        {
            details::heap_manager::run_build_task_synced(std::move(task));
        }
        else
        {
            const auto fallback_when_building = 
                weak_context->options.fallback_when_building;

            // Submit the task to the executor.
            details::heap_manager::submit_build_task(
                weak_context->executor,
                std::move(task)
            );

            // If the user wants to use a fallback, return the fallback.
            if(fallback_when_building)
            {
                // Creating accessor requires locking.
                entry_map_lock.lock();
                accessor = make_accessor(fallback_when_building, true);
            }
        }

        accessor.fetch_state();

        if constexpr(Transient)
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
        const auto state = accessor.last_state();

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
            if(options().rebuild_if_evicted)
                return build_resource<false>();
            if(options().fallback_if_evicted)
                return make_accessor(options().fallback_if_evicted, true);
        }
        // Failed state will be only changed here too.
        else if(state.failed()) [[unlikely]]
        {
            if(options().rebuild_if_failed)
                return build_resource<false>();
            if(options().fallback_if_failed)
                return make_accessor(options().fallback_if_failed, true);
        }
        // Otherwise, either the resource is being built, or the user doesn't
        // want to use a fallback. The state of the resource could be volatile.
        // The state that we just fetched will be returned to the user.
        return accessor;
    }

    auto process_request()
    {
        assert(context->executor);

        ensure_requested_descriptor();
        validate_builder_type();
        get_heap();
        init_accessor();

        return branch_on_resource_state();
    }

    auto process_request_transient()
    {
        assert(!context->executor);

        ensure_requested_descriptor();
        get_heap();
        init_accessor();

        // Manually simplified code path.
        if(accessor.last_state() == ResourceState::ABSENT_FIRST_REQUEST)
        {
            return build_resource<true>();
        }

        // todo enforce during compile time?
        USAGI_UNREACHABLE("Transient resource shouldn't be requested twice.");
    }
};

template <typename ResourceBuilderT, typename BuildArgFuncT>
ResourceRequestHandler(UniqueResourceRequestContext<
    ResourceBuilderT, BuildArgFuncT
> context) -> ResourceRequestHandler<ResourceBuilderT, BuildArgFuncT>;
}
