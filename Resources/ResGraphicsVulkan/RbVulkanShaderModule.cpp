#include "RbVulkanShaderModule.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

namespace usagi
{
RbVulkanShaderModule::RbVulkanShaderModule(
    std::string normalized_asset_path,
    const GpuShaderStage stage)
    : RbAssetDerivative(std::move(normalized_asset_path))
    , mStage(stage)
{
}

ResourceState RbVulkanShaderModule::construct(
    ResourceConstructDelegate<RbVulkanShaderModule> &delegate)
{
    // Get shader source code
    auto &bytecodes = delegate.resource<RbSpirvBytecodes>(
        asset_path(),
        mStage
    ).await().bytecodes();

    // Create the object
    vk::ShaderModuleCreateInfo info;

    info.pCode = bytecodes.data();
    info.codeSize = bytecodes.size() * sizeof(std::uint32_t);

    delegate.allocate(info, bytecodes);

    return ResourceState::READY;
}
}
