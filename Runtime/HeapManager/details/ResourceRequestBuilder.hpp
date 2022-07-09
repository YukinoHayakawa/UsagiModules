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

template <typename Builder, typename LazyBuildArgFunc>
class ResourceRequestBuilder
{
    using ContextT = UniqueResourceRequestContext<Builder, LazyBuildArgFunc>;
    using ProductT = typename Builder::ProductT;

    ContextT mContext;

    ResourceBuildOptions & options() { return mContext->options; }

public:
    explicit ResourceRequestBuilder(ContextT context)
        : mContext(std::move(context))
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

    ResourceAccessor<ProductT> make_request();
};

template <typename Builder, typename BuildArgFuncT>
ResourceRequestBuilder(UniqueResourceRequestContext<
    Builder, BuildArgFuncT
> context) -> ResourceRequestBuilder<Builder, BuildArgFuncT>;
}
