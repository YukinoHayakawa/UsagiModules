#pragma once

namespace usagi
{
template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ResourceBuilderT>::resource(
    Args &&...build_params)
{
    ResourceBuildOptions options;
    options.requesting_resource = mDescriptor;
    options.rebuild_if_failed = true;
    options.rebuild_if_evicted = true;
    return mManager->request_resource<AnotherBuilderT>(
        options,
        mExecutor,
        [&]{return std::forward_as_tuple(std::forward<Args>(build_params)...);}
    );
}
}
