﻿#pragma once

namespace usagi
{
template <typename ResourceBuilderT, typename LazyBuildArgFunc>
ResourceAccessor<typename ResourceBuilderT::ProductT>
ResourceRequestBuilder<ResourceBuilderT, LazyBuildArgFunc>::make_request()
{
    HeapManager *manager = mContext->manager;
    return manager->request_resource<ResourceBuilderT>(std::move(mContext));
}
}
