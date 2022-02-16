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
    , public VulkanDeviceAccess
    , VulkanObjectManager<VulkanShaderModule>
    , VulkanObjectManager<VulkanGraphicsPipeline>
    , VulkanObjectManager<VulkanCommandListGraphics>
{

    // Import the overloads into our scope so overload resolution works
    // properly.

    using VulkanObjectManager<VulkanShaderModule>::allocate_impl;
    using VulkanObjectManager<VulkanGraphicsPipeline>::allocate_impl;
    using VulkanObjectManager<VulkanCommandListGraphics>::allocate_impl;

    using VulkanObjectManager<VulkanShaderModule>::resource_impl;
    using VulkanObjectManager<VulkanGraphicsPipeline>::resource_impl;
    using VulkanObjectManager<VulkanCommandListGraphics>::resource_impl;

public:
    explicit HeapVulkanObjectManager(VulkanGpuDevice *device)
        : VulkanDeviceAccess(device)
    {
    }

    template <typename Object>
    auto & resource(const HeapResourceIdT id)
    {
        return resource_impl<Object>(id);
    }

    template <
        typename Object,
        typename CreateInfo,
        typename... Args
    >
    auto & allocate(
        const HeapResourceIdT id,
        CreateInfo &&create_info,
        Args &&...args)
    requires std::is_constructible_v<
        Object,
        decltype(VulkanDeviceAccess::create(std::declval<CreateInfo>())),
        Args...
    >
    {
        auto &object = allocate_impl<Object>(
            id,
            VulkanDeviceAccess::create(create_info),
            std::forward<Args>(args)...
        );

        // Inject device access if the object needs it.
        if constexpr(std::is_base_of_v<
            VulkanDeviceAccess,
            std::remove_cvref_t<decltype(object)>
        >) object.connect(this);

        return object;
    }
};
}
