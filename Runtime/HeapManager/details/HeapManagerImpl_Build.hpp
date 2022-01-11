#pragma once
#include "ResourceBuildTask.hpp"

namespace usagi
{

/*
template <typename ResourceBuilderT, typename BuildParamTupleT>
auto HeapManager::resource(
    const std::optional<HeapResourceDescriptor> resource_requester_id,
    const std::optional<HeapResourceDescriptor> resource_cache_id,
    BuildParamTupleT &&lazy_build_params)
-> ResourceAccessor<ResourceBuilderProductT<ResourceBuilderT>>
{
    // Current state: Unknown

    // If the resource id is provided, try whether it could be found in
    // the specified heap. If it exists, return it. Otherwise create an
    // entry for it and grab a reference, passing it to the builder.
    // So the resource won't be released before it has been built.
    // todo: the user might provide incorrect heap id that doesn't match with the heap used by the builder. what to do?
    if(resource_cache_id.has_value())
    {
        // if(const auto accessor = locate_cached_resource<
        //     ResourceBuilderProductT<ResourceBuilderT>
        // >(resource_requester_id, resource_cache_id.value()))
        // {
        //     return accessor.value();
        // }

        // Atomically get the record of the requested resource.
        auto [it, inserted] = try_emplace_resource(resource_cache_id.value());

        // Current state: > ABSENT

        // todo only one caller shall initiate the build task.

        // Locate the heap that is supposed to contain the requested resource.
        auto &heap = heap<typename ResourceBuilderT::TargetHeapT>(
            resource_cache_id->heap_id()
        );

        // todo if failed, reinit or return to user?

        // If the resource entry has been already created, return an accessor
        // through which the resource could be reached if it's ready.
        if(it->second.is_present())
        {
            return make_accessor<ResourceBuilderT>(
                &heap,
                resource_cache_id.resource_id()
            );
        }
    }
    std::unique_lock
    // If the resource is evicted and only has a tomb or not loaded at all,
    // construct a loader to load it.
    return std::apply(
        [this, rid = resource_requester_id]
        <typename... Args>
    (Args &&...args) {
            return build_resource<ResourceBuilderT>(
                rid,
                std::forward<Args>(args)...
            );
        }, lazy_build_params()
    );
}

template <typename ResourceBuilderT>
auto HeapManager::locate_cached_resource(
    const std::optional<HeapResourceDescriptor> resource_requester_id,
    const HeapResourceDescriptor resource_cache_id)
-> std::optional<ResourceBuilderProductT<ResourceBuilderT>>
{
    auto [it, inserted] = try_emplace_resource(resource_cache_id);

    // Locate the heap that is supposed to contain the requested resource.
    auto &heap = heap<ResourceBuilderT::TargetHeapT>(
        resource_cache_id.heap_id()
    );

    // todo if failed, reinit or return to user?

    // If the resource entry has been already created, return an accessor
    // through which the resource could be reached if it's ready.
    if(it->second.is_present())
    {
        return make_accessor<ResourceBuilderT>(
            &heap,
            resource_cache_id.resource_id()
        );
    }

    return build_with_it(it);

    // If it has, return it despite its state.
    if(accessor.resource_exists())
        return accessor;
    // todo, the target heap id specified by the user param must be checked against the builder if a build task is going to be created. if they don't match, throw.
}

template <typename ResourceBuilderT, typename ... Args>
auto HeapManager::build_resource(
    std::optional<HeapResourceDescriptor> resource_requester_id,
    Args &&... args)
-> ResourceAccessor<ResourceBuilderProductT<ResourceBuilderT>>
{
}*/
}
