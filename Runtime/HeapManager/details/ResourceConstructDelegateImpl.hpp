#pragma once

namespace usagi
{
template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ResourceBuilderT>::resource(
    Args &&...build_params)
    -> ResourceAccessor<AnotherBuilderT>
{
    auto requester = mContext->manager->template resource<AnotherBuilderT>(
        { },
        mContext->executor,
        [&]{return std::forward_as_tuple(std::forward<Args>(build_params)...);}
    );
    requester.requesting_from(mContext->entry->descriptor);
    requester.rebuild_if_failed();
    requester.rebuild_if_evicted();
    // Requesting resource from a resource builder always results in evaluation
    // of the parameters. So it doesn't matter if there are rvalue refs in
    // the parameters.
    return requester.make_request();
}
}
