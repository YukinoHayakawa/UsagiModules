#pragma once

#include <Usagi/Module/Common/Color/Color.hpp>

#include "Vulkan.hpp"
#include "VulkanEnum.hpp"

namespace usagi
{
class VulkanGpuDevice;

class VulkanCommandListGraphics
{
    VulkanGpuDevice *mDevice = nullptr;
    VulkanUniqueCommandBuffer mCommandBuffer;

public:
    // todo ctor

    void begin_recording();
    void end_recording();

    void clear_color_image(
        vk::Image image,
        Vulkan_GpuImageLayout layout,
        Color4f color);

    void image_transition(
        vk::Image image,
        Vulkan_GpuPipelineStage src_stage,
        Vulkan_GpuAccessMask src_access,
        Vulkan_GpuImageLayout old_layout,
        Vulkan_GpuPipelineStage dst_stage,
        Vulkan_GpuAccessMask dst_access,
        Vulkan_GpuImageLayout new_layout);

    operator vk::CommandBuffer()
    {
        return mCommandBuffer.get();
    }
};
}
