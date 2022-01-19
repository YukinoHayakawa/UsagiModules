#pragma once

#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetDerivative.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
class RbVulkanGraphicsPipeline : RbAssetDerivative
{
public:
    explicit RbVulkanGraphicsPipeline(std::string normalized_asset_path);

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanGraphicsPipeline;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanGraphicsPipeline> &delegate);
};
}
