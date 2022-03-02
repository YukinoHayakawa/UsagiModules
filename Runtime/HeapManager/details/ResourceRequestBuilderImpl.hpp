#pragma once

namespace usagi
{
template <typename ResourceBuilderT, typename LazyBuildArgFunc>
ResourceAccessor<ResourceBuilderT>
ResourceRequestBuilder<ResourceBuilderT, LazyBuildArgFunc>::make_request()
{
    return mContext->manager->template request_resource<ResourceBuilderT>(
        mContext
    );
}
}
