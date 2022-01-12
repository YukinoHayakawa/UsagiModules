#pragma once

namespace usagi
{
template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename BuildParamTupleFuncT>
auto ResourceConstructDelegate<ResourceBuilderT>::resource(
    const HeapResourceDescriptor resource_cache_id,
    BuildParamTupleFuncT &&build_params)
{
    ResourceBuildOptions options;
    options.requested_resource = resource_cache_id;
    options.requesting_resource = mDescriptor;
    options.rebuild_if_failed = true;
    options.rebuild_if_evicted = true;
    return mManager->request_resource<AnotherBuilderT>(
        options,
        mExecutor,
        std::forward<BuildParamTupleFuncT>(build_params)
    );
}
}
