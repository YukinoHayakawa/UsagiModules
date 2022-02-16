#pragma once

#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
class RbVulkanGraphicsPipeline : ArgumentStorage<AssetPath>
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanGraphicsPipeline;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanGraphicsPipeline> &delegate);
};
}
