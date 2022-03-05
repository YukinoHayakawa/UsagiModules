#pragma once

#include <thread>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "Vulkan.hpp"
#include "VulkanGraphicsPipeline.hpp"

namespace usagi
{
class NativeWindow;
class VulkanGraphicsPipelineCompiler;
class VulkanGpuDevice;

class VulkanDeviceAccess : Noncopyable
{
    VulkanGpuDevice *mDevice = nullptr;

protected:
    VulkanDeviceAccess() = default;
    explicit VulkanDeviceAccess(VulkanGpuDevice *device);

    VulkanUniqueShaderModule create(
        const vk::ShaderModuleCreateInfo &create_info) const;
    VulkanUniqueDescriptorSetLayout create(
        const vk::DescriptorSetLayoutCreateInfo &create_info) const;
    VulkanUniquePipelineLayout create(
        const vk::PipelineLayoutCreateInfo &create_info) const;
    VulkanUniqueRenderPass create(
        const vk::RenderPassCreateInfo &create_info) const;
    std::pair<VulkanUniquePipeline, vk::Result> create(
        const vk::GraphicsPipelineCreateInfo &create_info) const;
    VulkanUniqueSwapchain create(
        const vk::SwapchainCreateInfoKHR &create_info) const;
    VulkanUniqueSurface create(NativeWindow *window) const;
    VulkanGraphicsPipeline create(
        VulkanGraphicsPipelineCompiler &compiler) const;
    // VulkanUniqueCommandBuffer create(std::thread::id thread_id) const;

    const vk::DispatchLoaderDynamic & dispatch() const;

    VulkanGpuDevice * device() const { return mDevice; }

    vk::Queue present_queue() const;
    vk::Queue graphics_queue() const;

public:
    void connect(const VulkanDeviceAccess *another);
};

class VulkanDeviceExternalAccessProvider : VulkanDeviceAccess
{
    // Only allow VulkanDeviceAccess to instantiate this class.
    friend class VulkanDeviceAccess;
    using VulkanDeviceAccess::VulkanDeviceAccess;

public:
    using VulkanDeviceAccess::create;
};
}
