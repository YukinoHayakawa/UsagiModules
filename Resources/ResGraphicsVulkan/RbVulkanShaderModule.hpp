#pragma once

#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
class RbVulkanShaderModule
    : ArgumentStorage<
        AssetPath,
        GpuShaderStage
    >
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanShaderModule;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanShaderModule> &delegate);
};
}
