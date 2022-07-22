#include "VulkanDeviceAccess.hpp"

#include "VulkanGpuDevice.hpp"
#include "VulkanGraphicsPipelineCompiler.hpp"

namespace usagi
{
VulkanDeviceAccess::VulkanDeviceAccess(VulkanGpuDevice *device)
    : mDevice(device)
{
}

VulkanUniqueShaderModule VulkanDeviceAccess::create(
    const vk::ShaderModuleCreateInfo &create_info) const
{
    assert(mDevice);
    return mDevice->create_shader_module(create_info);
}

VulkanUniqueDescriptorSetLayout
VulkanDeviceAccess::create(
    const vk::DescriptorSetLayoutCreateInfo &create_info) const
{
    assert(mDevice);
    return mDevice->create_descriptor_set_layout(create_info);
}

VulkanUniquePipelineLayout VulkanDeviceAccess::create(
    const vk::PipelineLayoutCreateInfo &create_info) const
{
    assert(mDevice);
    return mDevice->create_pipeline_layout(create_info);
}

VulkanUniqueRenderPass VulkanDeviceAccess::create(
    const vk::RenderPassCreateInfo &create_info) const
{
    assert(mDevice);
    return mDevice->create_render_pass(create_info);
}

std::pair<VulkanUniquePipeline, vk::Result>
VulkanDeviceAccess::create(
    const vk::GraphicsPipelineCreateInfo &create_info) const
{
    assert(mDevice);
    return mDevice->create_graphics_pipeline(create_info);
}

VulkanUniqueSwapchain VulkanDeviceAccess::create(
    const vk::SwapchainCreateInfoKHR &create_info) const
{
    assert(mDevice);
    return mDevice->create_swapchain(create_info);
}

VulkanUniqueSurface VulkanDeviceAccess::create(NativeWindow *window) const
{
    assert(mDevice);
    return mDevice->create_surface(window);
}

/*
VulkanGraphicsPipeline VulkanDeviceAccess::create(
    VulkanGraphicsPipelineCompiler &compiler) const
{
    assert(mDevice);
    return compiler.compile(VulkanDeviceExternalAccessProvider(mDevice));
}
*/

// VulkanUniqueCommandBuffer VulkanDeviceAccess::create(
//     const std::thread::id thread_id) const
// {
//     return mDevice->allocate_graphics_command_list();
// }

const vk::DispatchLoaderDynamic & VulkanDeviceAccess::dispatch() const
{
    assert(mDevice);
    return mDevice->dispatch();
}

vk::Queue VulkanDeviceAccess::present_queue() const
{
    assert(mDevice);
    return mDevice->present_queue();
}

vk::Queue VulkanDeviceAccess::graphics_queue() const
{
    assert(mDevice);
    return mDevice->graphics_queue();
}

void VulkanDeviceAccess::connect(const VulkanDeviceAccess *another)
{
    assert(!mDevice && "Target device should only be set once.");
    mDevice = another->device();
}
}
