#pragma once

#include <vulkan/vulkan.hpp>

namespace usagi
{
using VulkanUniqueInstance =
    vk::UniqueHandle<vk::Instance, vk::DispatchLoaderDynamic>;
using VulkanUniqueDevice =
    vk::UniqueHandle<vk::Device, vk::DispatchLoaderDynamic>;
using VulkanUniqueSemaphore =
    vk::UniqueHandle<vk::Semaphore, vk::DispatchLoaderDynamic>;
using VulkanUniqueFence =
    vk::UniqueHandle<vk::Fence, vk::DispatchLoaderDynamic>;
using VulkanUniqueCommandBuffer =
    vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>;
using VulkanUniqueSurface =
    vk::UniqueHandle<vk::SurfaceKHR, vk::DispatchLoaderDynamic>;
using VulkanUniqueSwapchain =
    vk::UniqueHandle<vk::SwapchainKHR, vk::DispatchLoaderDynamic>;
}
