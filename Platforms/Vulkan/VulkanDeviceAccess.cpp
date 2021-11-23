#include "VulkanDeviceAccess.hpp"

#include "VulkanGpuDevice.hpp"

namespace usagi
{
VulkanDeviceAccess::VulkanDeviceAccess(VulkanGpuDevice *device)
    : mDevice(device)
{
}

VulkanUniqueShaderModule VulkanDeviceAccess::create_shader_module(
    const vk::ShaderModuleCreateInfo &create_info)
{
    return mDevice->create_shader_module(create_info);
}

VulkanUniqueDescriptorSetLayout
VulkanDeviceAccess::create_descriptor_set_layout(
    const vk::DescriptorSetLayoutCreateInfo &create_info)
{
    return mDevice->create_descriptor_set_layout(create_info);
}

VulkanUniquePipelineLayout VulkanDeviceAccess::create_pipeline_layout(
    const vk::PipelineLayoutCreateInfo &create_info)
{
    return mDevice->create_pipeline_layout(create_info);
}

VulkanUniqueRenderPass VulkanDeviceAccess::create_render_pass(
    const vk::RenderPassCreateInfo &create_info)
{
    return mDevice->create_render_pass(create_info);
}

std::pair<VulkanUniquePipeline, vk::Result>
VulkanDeviceAccess::create_graphics_pipeline(
    const vk::GraphicsPipelineCreateInfo &create_info)
{
    return mDevice->create_graphics_pipeline(create_info);
}

VulkanUniqueSwapchain VulkanDeviceAccess::create_swapchain(
    const vk::SwapchainCreateInfoKHR &create_info)
{
    return mDevice->create_swapchain(create_info);
}

const vk::DispatchLoaderDynamic & VulkanDeviceAccess::dispatch() const
{
    return mDevice->dispatch();
}

vk::Queue VulkanDeviceAccess::present_queue() const
{
    return mDevice->present_queue();
}

vk::Queue VulkanDeviceAccess::graphics_queue() const
{
    return mDevice->graphics_queue();
}
}
