#pragma once

#include <utility>

#include "HeapResourceDescriptor.hpp"
#include "ResourceBuildOptions.hpp"
#include "ResourceAccessor.hpp"

namespace usagi
{
class TaskExecutor;
class HeapManager;

template <typename ResourceBuilderT, typename BuildParamTupleFunc>
class ResourceRequestBuilder
{
    ResourceBuildOptions mOptions;
    BuildParamTupleFunc mParamFunc;
    HeapManager *mManager = nullptr;
    TaskExecutor *mExecutor = nullptr;

    friend class HeapManager;

public:
    ResourceRequestBuilder(
        HeapManager *manager,
        TaskExecutor *executor,
        const HeapResourceDescriptor resource_cache_id,
        BuildParamTupleFunc param_func)
        : mParamFunc(std::move(param_func))
        , mManager(manager)
        , mExecutor(executor)
    {
        mOptions.requested_resource = resource_cache_id;
    }

    auto & fallback_when_building(const HeapResourceDescriptor cache_id)
    {
        mOptions.fallback_when_building = cache_id;
        return *this;
    }

    auto & rebuild_if_failed()
    {
        mOptions.rebuild_if_failed = true;
        return *this;
    }

    auto & fallback_if_failed(const HeapResourceDescriptor cache_id)
    {
        mOptions.fallback_if_failed = cache_id;
        return *this;
    }

    // todo handle thrashing
    auto & rebuild_if_evicted()
    {
        mOptions.rebuild_if_evicted = true;
        return *this;
    }

    auto & fallback_if_evicted(const HeapResourceDescriptor cache_id)
    {
        mOptions.fallback_if_evicted = cache_id;
        return *this;
    }

    auto make_request() -> ResourceAccessor<ResourceBuilderT>;
};
}
