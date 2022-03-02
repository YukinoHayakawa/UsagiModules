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
    // is not added to HeapTypes.
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
        ResourceBuilder ResourceBuilderT,
        typename... BuildArgs
    >
    decltype(auto) resource_transient(BuildArgs &&... args)
    // Adds a constraint that checks whether the required heap is added.
    requires IsTargetHeapRegistered<ResourceBuilderT>
    {
        return HeapManager::resource_transient<ResourceBuilderT>(
            std::forward<BuildArgs>(args)...
        );
    }

    template <
        ResourceBuilder ResourceBuilderT,
        typename BuildParamTupleFuncT
    >
    decltype(auto) resource(
        HeapResourceDescriptor resource_cache_id,
        TaskExecutor *executor,
        BuildParamTupleFuncT &&lazy_build_params)
    // Adds a constraint that checks whether the required heap is added.
    requires IsTargetHeapRegistered<ResourceBuilderT>
    {
        return HeapManager::resource<ResourceBuilderT>(
            resource_cache_id,
            executor,
            std::forward<BuildParamTupleFuncT>(lazy_build_params)
        );
    }

    // Service access interface.
    auto & heap_manager()
    {
        return *this;
    }
};
}
