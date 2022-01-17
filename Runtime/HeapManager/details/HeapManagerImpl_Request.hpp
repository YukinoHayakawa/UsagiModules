#pragma once

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
>
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
    std::optional<ResourceBuilderT> builder;

    // If the resource id is unknown, we must construct the builder to obtain
    // it.
    if(!options.requested_resource)
    {
        // Result from std::forward_as_tuple()
        auto param_tuple = param_func();

        // Building the hash won't alter the content of the tuple.
        const auto res_id = std::apply(
            [&]<typename... Args>(Args &&...args) {
                return make_resource_id<ResourceBuilderT>(
                    std::forward<Args>(args)...
                );
            }, param_tuple
        );

        // Init the builder to get the heap ID.
        std::apply(
            [&]<typename... Args>(Args &&...args) {
                builder.emplace(std::forward<Args>(args)...);
            }, param_tuple
        );
        // const auto heap_id = builder->target_heap();
        // todo
        const auto heap_id = typeid(TargetHeapT).hash_code();

        descriptor = { heap_id, res_id };
    }
    else
    {
        descriptor = options.requested_resource;
    } // todo validate builderT

    // ====================== Enter Critical Section ======================== //
    // Read/Write the resource entry table.

    std::unique_lock lk(mEntryMapMutex);

    // Try to get the heap. The heap must exist before the resource
    // could be fetched or built. If this fails, exception will be thrown.
    auto *heap = locate_heap<TargetHeapT>();

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

    auto build = [&]() -> decltype(auto)
    {
        // When turned into preparing state, it's guaranteed that no another
        // build task will be created.
        accessor.mEntry->state.store(
            ResourceState::PREPARING,
            std::memory_order::release
        );

        // Create the promise outside the construction of the builder
        // to shorten the critical section.
        std::promise<void> promise;
        accessor.mEntry->future = promise.get_future();

        // Make sure that the future is accessible from now on.
        lk.unlock();

        LOG(info,
            "[Heap] Building resource: {} (builder={}, resource={})",
            descriptor,
            typeid(ResourceBuilderT).name(),
            typeid(typename ResourceBuilderT::ProductT).name()
        );

        // Create build task.
        std::unique_ptr<ResourceBuildTask<ResourceBuilderT>> task;

        auto make_build_task_ = [&]<typename... Args>(Args &&...args)
        {
            return std::make_unique<ResourceBuildTask<ResourceBuilderT>>(
                this,
                executor,
                accessor,
                std::move(promise),
                std::forward<Args>(args)...
            );
        };

        auto make_task = [&]<typename T>(T &&params) {
            return std::apply(make_build_task_, params);
        };

        if(builder.has_value())
            // If the builder is already constructed, move construct it.
            task = make_task(std::forward_as_tuple(std::move(builder.value())));
        else
            task = make_task(param_func());

        // Submit the task to the executor.
        submit_build_task(executor, std::move(task));
        accessor.mEntry->state.store(
            ResourceState::SCHEDULED,
            std::memory_order::release
        );

        // If the user wants to use a fallback, return the fallback.
        if(options.fallback_when_building)
        {
            lk.lock();
            return make_res_accessor(options.fallback_when_building, true);
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
