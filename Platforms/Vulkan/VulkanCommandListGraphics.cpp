﻿#include "VulkanCommandListGraphics.hpp"

#include "VulkanGpuDevice.hpp"

namespace usagi
{
VulkanCommandListGraphics::VulkanCommandListGraphics(
    VulkanUniqueCommandBuffer command_buffer)
    : mCommandBuffer(std::move(command_buffer))
{
    assert(mCommandBuffer);
}

void VulkanCommandListGraphics::check_buffer() const
{
    assert(mCommandBuffer && "Command buffer ownership transferred?");
}

VulkanCommandListGraphics & VulkanCommandListGraphics::begin_recording()
{
    check_buffer();
    assert(!mRecording);

    vk::CommandBufferBeginInfo info;
    // This command buffer will only be used once. It will be reset before
    // next submission and after the last execution is finished.
    info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    mCommandBuffer->begin(info, dispatch());

    mRecording = true;

    return *this;
}

VulkanCommandListGraphics & VulkanCommandListGraphics::end_recording()
{
    check_buffer();
    assert(mRecording);

    mCommandBuffer->end(dispatch());

    mRecording = false;

    return *this;
}

// Note: bad performance on tile-based GPUs
// https://developer.samsung.com/game/usage#clearingattachments
VulkanCommandListGraphics & VulkanCommandListGraphics::clear_color_image(
    const vk::Image image,
    const Vulkan_GpuImageLayout layout,
    Color4f color)
{
    check_buffer();

    // todo: depending on image format, float/uint/int should be used
    const vk::ClearColorValue color_value { std::array {
        color.x(), color.y(), color.z(), color.w()
    }};
    vk::ImageSubresourceRange subresource_range;
    subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subresource_range.setBaseArrayLayer(0);
    subresource_range.setLayerCount(1);
    subresource_range.setBaseMipLevel(0);
    subresource_range.setLevelCount(1);
    mCommandBuffer->clearColorImage(
        image,
        layout,
        color_value,
        { subresource_range },
        dispatch()
    );

    return *this;
}

// Synchronization:
// https://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
// https://github.com/philiptaylor/vulkan-sxs/blob/master/04-clear/README.md
// https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/extensions/VK_KHR_synchronization2.md
// https://www.khronos.org/blog/understanding-vulkan-synchronization
VulkanCommandListGraphics & VulkanCommandListGraphics::image_transition(
    const vk::Image image,
    const Vulkan_GpuPipelineStage src_stage,
    const Vulkan_GpuAccessMask src_access,
    const Vulkan_GpuImageLayout old_layout,
    const Vulkan_GpuPipelineStage dst_stage,
    const Vulkan_GpuAccessMask dst_access,
    const Vulkan_GpuImageLayout new_layout)
{
check_buffer();

    vk::ImageMemoryBarrier2KHR barrier;

    barrier.setImage(image);
    barrier.setOldLayout(old_layout);
    barrier.setNewLayout(new_layout);
    barrier.setSrcAccessMask(src_access);
    barrier.setDstAccessMask(dst_access);
    barrier.setSrcStageMask(src_stage);
    barrier.setDstStageMask(dst_stage);

    // todo fix
    const auto queue_family_index = 1;
    // = mCommandPool->device()->graphicsQueueFamily();
    barrier.setSrcQueueFamilyIndex(queue_family_index);
    barrier.setDstQueueFamilyIndex(queue_family_index);

    barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(1);
    barrier.subresourceRange.setBaseMipLevel(0);
    barrier.subresourceRange.setLevelCount(1);

    vk::DependencyInfoKHR dep;

    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &barrier;

    mCommandBuffer->pipelineBarrier2KHR(dep, dispatch());

    return *this;
}
}
