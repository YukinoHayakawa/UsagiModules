#pragma once

#include "HeapManager.hpp"

namespace usagi
{
// todo this impl is temp
template <typename... HeapTypes>
class HeapManagerStatic : HeapManager
{
    using ImplementedHeaps = std::tuple<HeapTypes...>;

    // If this evaluates to false, it means that BuilderT::TargetHeapT
    // is not added to HeapTypes, or definition of BuilderT is not complete.
    template <ResourceBuilder BuilderT>
    constexpr static bool IsTargetHeapRegistered =
        has_type_v<typename BuilderT::TargetHeapT, ImplementedHeaps>;

public:
    template <typename... ArgTuples>
    explicit HeapManagerStatic(ArgTuples &&... arg_tuples)
        requires (... && ConstructibleFromTuple<HeapTypes, ArgTuples>)
    {
        (..., USAGI_APPLY(add_heap<HeapTypes>, arg_tuples));
    }

    // It would be much easier if C++ supports some kind of decorators.
    template <
        ResourceBuilder Builder,
        typename... BuildArgs
    >
    auto resource_transient(BuildArgs &&... args)
    // Adds a constraint that checks whether the required heap is added.
    requires (is_type_complete_v<Builder> && IsTargetHeapRegistered<Builder>)
    {
        return HeapManager::resource_transient<Builder>(
            std::forward<BuildArgs>(args)...
        );
    }

    /*
    USAGI_APPEND_CONSTRAINTS(
        HeapManager, 
        resource_transient, 
        IsTargetHeapRegistered<ResourceBuilderT>,
        typename ResourceBuilderT
    );
    */

    template <
        ResourceBuilder Builder,
        typename LazyBuildArgFunc
    >
    ResourceRequestBuilder<Builder, LazyBuildArgFunc> resource(
        HeapResourceDescriptor resource_cache_id,
        TaskExecutor *executor,
        LazyBuildArgFunc &&lazy_build_params)
    // Adds a constraint that checks whether the required heap is added.
    // bug is_type_complete_v seems not working
    requires (is_type_complete_v<Builder> && IsTargetHeapRegistered<Builder>)
    {
        return HeapManager::resource<Builder>(
            resource_cache_id,
            executor,
            std::forward<LazyBuildArgFunc>(lazy_build_params)
        );
    }

    // Service access interface.
    auto & heap_manager()
    {
        return *this;
    }
};
}
