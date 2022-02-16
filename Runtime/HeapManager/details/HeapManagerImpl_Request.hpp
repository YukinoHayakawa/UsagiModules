#pragma once

#include <Usagi/Library/Utilities/Functional.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
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
auto HeapManager::resource_immediate(BuildArgs &&...args)
-> ResourceAccessor<ResourceBuilderT>
requires std::constructible_from<ResourceBuilderT, BuildArgs...>
{
    auto params = [&] {
        return std::forward_as_tuple(std::forward<BuildArgs>(args)...);
    };

    return ResourceRequestBuilder<ResourceBuilderT, decltype(params)> {
        this,
        nullptr,
        { },
        std::move(params)
    }.make_request();
}

template <ResourceBuilder ResourceBuilderT, typename BuildParamTupleFunc>
auto HeapManager::request_resource(
    const ResourceBuildOptions &options,
    TaskExecutor *executor,
    BuildParamTupleFunc &&param_func)
-> ResourceAccessor<ResourceBuilderT>
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    // using ResourceT = typename ResourceBuilderT::ProductT;

    // todo reg dependency

    HeapResourceDescriptor descriptor;
    std::optional<decltype(param_func())> param_tuple;

    // Build resource id.

    // If the resource id is unknown, we must construct the builder to obtain
    // it.
    if(!options.requested_resource)
    {
        // Result from std::forward_as_tuple()/make_tuple()
        // Be sure that the parameters returned by the tuple refer to valid
        // memory addresses.
        param_tuple.emplace(param_func());

        // Hash the builder type & parameters to get the id of the resource.
        // Building the hash won't alter the content of the tuple.
        descriptor = USAGI_APPLY(
            make_resource_descriptor<ResourceBuilderT>,
            param_tuple.value()
        );
    }
    else
    {
        descriptor = options.requested_resource;

        // Validate builder type.
        USAGI_ASSERT_THROW(
            descriptor.heap_id() == typeid(TargetHeapT).hash_code(),
            std::runtime_error("Builder type doesn't match with the one "
                "used to create the resource.")
        );
    }

    // Try to get the heap. The heap must exist before the resource
    // could be fetched or built. If this fails, exception will be thrown.
    auto *heap = locate_heap<TargetHeapT>();

    // ====================== Enter Critical Section ======================== //
    // Read/Write the resource entry table.
    // Resource entry is also used to track the ref count of the resources.

    std::unique_lock lk(mEntryMapMutex);

    // Return an accessor to the specified resource or a proper fallback.
    auto make_res_accessor = [&](
        const HeapResourceDescriptor desc,
        const bool is_fallback)
    {
        return make_accessor_nolock<ResourceBuilderT>(
            desc,
            heap,
            is_fallback
        );
    };

    auto accessor = make_res_accessor(descriptor, false);
    const auto state = accessor.last_state();

    // Construct the build task and submit it.
    auto build = [&]() -> decltype(auto)
    {
        // When turned into preparing state, it's guaranteed that no another
        // build task will be created.
        accessor.mEntry->state.store(
            ResourceState::PREPARING,
            std::memory_order::release
        );

        // Create the promise outside the construction of the builder task
        // to shorten the critical section.
        std::promise<void> promise;
        accessor.mEntry->future = promise.get_future();

        // The future object is accessible from now on and the resource entry
        // enters a state that the entry will not be recreated/removed.
        // Safe to unlock here. Our accessor holds a ref to it so it won't be
        // removed.
        lk.unlock();

        LOG(info,
            "[Heap] Building resource: {} (builder={}, resource={})",
            descriptor,
            typeid(ResourceBuilderT).name(),
            typeid(typename ResourceBuilderT::ProductT).name()
        );

        if(!param_tuple) param_tuple.emplace(param_func());

        // Create build task.
        std::unique_ptr<ResourceBuildTask<ResourceBuilderT>> task;
        task = std::apply([&]<typename... Args>(Args &&...args)
        {
            // todo pool the task objects
            return std::make_unique<ResourceBuildTask<ResourceBuilderT>>(
                this,
                executor,
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
        if(!executor)
        {
            run_build_task_synced(std::move(task));

            // Validate asset state.
            accessor.fetch_state();
            assert(accessor.last_state().failed() ||
                accessor.last_state().ready());
        }
        else
        {
            // Submit the task to the executor.
            submit_build_task(executor, std::move(task));

            // If the user wants to use a fallback, return the fallback.
            if(options.fallback_when_building)
            {
                // Creating accessor requires locking.
                lk.lock();
                return make_res_accessor(options.fallback_when_building, true);
            }
        }

        // Otherwise, return the current state.
        accessor.fetch_state();
        return accessor;
    };

    // Ready state won't change when in critical section.
    if(state.ready())
    {
        return accessor;
    }
    // If this is the first request, build the resource.
    // ABSENT state won't change either. Only critical sections can alter it.
    if(state == ResourceState::ABSENT_FIRST_REQUEST)
    {
        return build();
    }
    // If the resource has been evicted, see whether to rebuild or use
    // fallback.
    if(state == ResourceState::ABSENT_EVICTED)
    {
        if(options.rebuild_if_evicted)
            return build();
        if(options.fallback_if_evicted)
            return make_res_accessor(options.fallback_if_evicted, true);
    }
    // Failed state will be only changed here too.
    else if(state.failed())
    {
        if(options.rebuild_if_failed)
            return build();
        if(options.fallback_if_failed)
            return make_res_accessor(options.fallback_if_failed, true);
    }
    // Otherwise, either the resource is being built, or the user doesn't
    // want to use a fallback. The state of the resource could be volatile.
    // The state that we just fetched will be returned to the user.
    return accessor;
}
}
