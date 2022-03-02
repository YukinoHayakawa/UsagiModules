#pragma once

#include <utility>

#include "HeapResourceDescriptor.hpp"
#include "ResourceBuildOptions.hpp"
#include "ResourceAccessor.hpp"
#include "ResourceRequestContext.hpp"

namespace usagi
{
class TaskExecutor;
class HeapManager;

template <typename ResourceBuilderT, typename LazyBuildArgFunc>
class ResourceRequestBuilder
{
    // friend class HeapManager;

    using ContextT = ResourceRequestContext<ResourceBuilderT, LazyBuildArgFunc>;

    ContextT *mContext = nullptr;

    ResourceBuildOptions & options() { return mContext->options; }

public:
    explicit ResourceRequestBuilder(ContextT *context)
        : mContext(context)
    {
    }

    auto & requesting_from(const HeapResourceDescriptor id)
    {
        options().requesting_resource = id;
        return *this;
    }

    auto & fallback_when_building(const HeapResourceDescriptor cache_id)
    {
        options().fallback_when_building = cache_id;
        return *this;
    }

    auto & rebuild_if_failed()
    {
        options().rebuild_if_failed = true;
        return *this;
    }

    auto & fallback_if_failed(const HeapResourceDescriptor cache_id)
    {
        options().fallback_if_failed = cache_id;
        return *this;
    }

    // todo handle thrashing
    auto & rebuild_if_evicted()
    {
        options().rebuild_if_evicted = true;
        return *this;
    }

    auto & fallback_if_evicted(const HeapResourceDescriptor cache_id)
    {
        options().fallback_if_evicted = cache_id;
        return *this;
    }

    ResourceAccessor<ResourceBuilderT> make_request();
};
}
