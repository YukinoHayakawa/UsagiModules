#pragma once

#include "Vulkan.hpp"

namespace usagi
{
class VulkanGpuDevice;

class VulkanDeviceAccess
{
protected:
    VulkanGpuDevice *mDevice = nullptr;

    explicit VulkanDeviceAccess(VulkanGpuDevice *device);

    VulkanUniqueShaderModule create_shader_module(
        const vk::ShaderModuleCreateInfo &create_info);
    VulkanUniqueDescriptorSetLayout create_descriptor_set_layout(
        const vk::DescriptorSetLayoutCreateInfo &create_info);
    VulkanUniquePipelineLayout create_pipeline_layout(
        const vk::PipelineLayoutCreateInfo &create_info);
    VulkanUniqueRenderPass create_render_pass(
        const vk::RenderPassCreateInfo &create_info);
    std::pair<VulkanUniquePipeline, vk::Result> create_graphics_pipeline(
        const vk::GraphicsPipelineCreateInfo &create_info);
    VulkanUniqueSwapchain create_swapchain(
        const vk::SwapchainCreateInfoKHR &create_info);

    const vk::DispatchLoaderDynamic & dispatch() const;

    vk::Queue present_queue() const;
    vk::Queue graphics_queue() const;
};
}
