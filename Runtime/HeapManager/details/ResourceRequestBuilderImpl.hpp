#pragma once

namespace usagi
{
template <typename ResourceBuilderT, typename BuildParamTupleFunc>
auto ResourceRequestBuilder<ResourceBuilderT, BuildParamTupleFunc>::
make_request()
-> ResourceAccessor<ResourceBuilderT>
{
    return mManager->request_resource<ResourceBuilderT>(
        mOptions,
        mExecutor,
        std::move(mParamFunc)
    );
}
}
