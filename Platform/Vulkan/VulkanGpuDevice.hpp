#pragma once

#include <map>
#include <mutex>

#include <Usagi/Module/Service/Windowing/NativeWindow.hpp>

#include "Vulkan.hpp"
#include "VulkanCommandListGraphics.hpp"
#include "VulkanSwapchain.hpp"

namespace usagi
{
class VulkanGpuDevice
{
    // keeps the connection to vulkan dynamic library
    vk::DynamicLoader mLoader;
    vk::DispatchLoaderDynamic mDispatchInstance;
    vk::DispatchLoaderDynamic mDispatchDevice;
    VulkanUniqueInstance mInstance;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>
        mDebugUtilsMessenger;
    vk::PhysicalDevice mPhysicalDevice;
    VulkanUniqueDevice mDevice;

    vk::Queue mGraphicsQueue;
    std::uint32_t mGraphicsQueueFamilyIndex = -1;
    std::vector<vk::CommandBufferSubmitInfoKHR> mCmdSubmitInfo;

    static uint32_t select_queue(
        std::vector<vk::QueueFamilyProperties> &queue_family,
        const vk::QueueFlags &queue_flags);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback_dispatcher(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
        void *user_data);

    VkBool32 debug_messenger_callback(
        const vk::DebugUtilsMessageSeverityFlagsEXT &message_severity,
        const vk::DebugUtilsMessageTypeFlagsEXT &message_type,
        const vk::DebugUtilsMessengerCallbackDataEXT *callback_data) const;

    void create_instance();
    void create_debug_report();
    void select_physical_device();
    void create_device_and_queues();

    // swapchain & presentation

    void check_queue_presentation_capacity_throw(
        uint32_t queue_family_index) const;
    static const char * platform_surface_extension_name();

    std::map<NativeWindow *, std::unique_ptr<VulkanSwapchain>> mSwapchainCache;
    VulkanSwapchain & create_swapchain(NativeWindow *window);

public:
    VulkanGpuDevice();
    ~VulkanGpuDevice();

    VulkanCommandListGraphics thread_allocate_graphics_command_list();

    // queue submission workflow

    std::lock_guard<std::mutex> thread_submission_lock();

    // todo lock
    void submit_graphics_jobs(
        // the command lists are allocated to rendering systems via graphics
        // services whose implementation type is determined during compile
        // time. the systems should use auto types to store the command lists,
        // so they can be passed to this parameter without casting.
        std::span<VulkanCommandListGraphics> cmd_lists,
        std::span<vk::Semaphore> wait_semaphores,
        // each stage flag corresponds to one semaphore
        std::span<Vulkan_GpuPipelineStage> wait_stages,
        std::span<vk::Semaphore> signal_semaphores,
        // each stage flag corresponds to one semaphore
        std::span<Vulkan_GpuPipelineStage> signal_stages,
        vk::Fence signal_fence = { }
    );

    void submit_graphics_jobs(
        std::span<VulkanCommandListGraphics> cmd_lists,
        std::span<vk::Semaphore> wait_semaphores,
        std::span<GpuPipelineStage> wait_stages,
        std::span<vk::Semaphore> signal_semaphores,
        std::span<GpuPipelineStage> signal_stages,
        vk::Fence signal_fence = { }
    );

    void thread_queue_submission_unlock();


    // todo: implement semaphore pool
    VulkanUniqueSemaphore allocate_semaphore();
    VulkanUniqueFence allocate_fence();

    VulkanSwapchain & swapchain(NativeWindow *window);
    void destroy_swapchain(NativeWindow *window);

    vk::PhysicalDevice physical_device() const { return mPhysicalDevice; }
    vk::Device device() const { return mDevice.get(); }
    vk::Queue present_queue() const { return mGraphicsQueue; }

    const vk::DispatchLoaderDynamic & dispatch_device() const
    {
        return mDispatchDevice;
    }

    const vk::DispatchLoaderDynamic & dispatch_instance() const
    {
        return mDispatchInstance;
    }
};
}
