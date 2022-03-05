#pragma once

#include <Usagi/Modules/Common/Color/Color.hpp>

#include "VulkanDeviceAccess.hpp"
#include "VulkanEnum.hpp"

namespace usagi
{
class VulkanCommandListGraphics : public VulkanDeviceAccess
{
    friend class VulkanGpuDevice;

    VulkanUniqueCommandBuffer mCommandBuffer;
    bool mRecording = false;

    void check_buffer() const;

public:
    VulkanCommandListGraphics(VulkanUniqueCommandBuffer command_buffer);

    VulkanCommandListGraphics & begin_recording();
    VulkanCommandListGraphics & end_recording();

    VulkanCommandListGraphics & clear_color_image(
        vk::Image image,
        Vulkan_GpuImageLayout layout,
        Color4f color);

    VulkanCommandListGraphics & image_transition(
        vk::Image image,
        Vulkan_GpuPipelineStage src_stage,
        Vulkan_GpuAccessMask src_access,
        Vulkan_GpuImageLayout old_layout,
        Vulkan_GpuPipelineStage dst_stage,
        Vulkan_GpuAccessMask dst_access,
        Vulkan_GpuImageLayout new_layout);
};
}
