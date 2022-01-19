#pragma once

#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>
#include <Usagi/Modules/Resources/ResGraphicsCommon/RbSpirvBytecodes.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
class RbVulkanShaderModule : RbAssetDerivative
{
    std::string mNormalizedAssetPath;
    GpuShaderStage mStage;

public:
    RbVulkanShaderModule(
        std::string normalized_asset_path,
        GpuShaderStage stage);

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanShaderModule;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanShaderModule> &delegate);
};
}
