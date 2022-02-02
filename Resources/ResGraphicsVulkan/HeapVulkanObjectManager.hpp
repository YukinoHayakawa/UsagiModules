#pragma once

#include <Usagi/Modules/Runtime/HeapManager/Heap.hpp>
#include <Usagi/Modules/Runtime/HeapManager/details/HeapResourceDescriptor.hpp>

#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipelineCompiler.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>

#include "VulkanObjectManager.hpp"

namespace usagi
{
class HeapVulkanObjectManager
    : public Heap
    , VulkanDeviceAccess
    , VulkanObjectManager<VulkanShaderModule>
    , VulkanObjectManager<VulkanGraphicsPipeline>
    , VulkanObjectManager<VulkanUniqueCommandPool>
{
    // // todo: how to know which threads are dead? make threads into resources?
    // std::map<std::thread::id, VulkanCommandPool> mCommandPools;

public:
    explicit HeapVulkanObjectManager(VulkanGpuDevice *device)
        : VulkanDeviceAccess(device)
    {
    }

    template <typename Object>
    const auto & resource(const HeapResourceIdT id)
    {
        return VulkanObjectManager<Object>::resource(id);
    }

    template <typename Object, typename CreateInfo, typename... Args>
    const auto & allocate(
        const HeapResourceIdT id,
        CreateInfo create_info,
        Args &&...args)
    requires std::is_constructible_v<
        Object,
        decltype( VulkanDeviceAccess::create(create_info)), Args...
    >
    {
        return VulkanObjectManager<Object>::allocate(
            id,
            VulkanDeviceAccess::create(create_info),
            std::forward<Args>(args)...
        );
    }
};
}
