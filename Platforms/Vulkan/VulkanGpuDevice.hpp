#pragma once

#include <deque>
#include <map>
#include <mutex>

#include "VulkanCommandListGraphics.hpp"
#include "VulkanSwapchain.hpp"

namespace usagi
{
class NativeWindow;

class VulkanGpuDevice
{
    friend class VulkanDeviceAccess;

public:
    class SemaphoreInfo
    {
        friend class VulkanGpuDevice;

        std::vector<VulkanUniqueSemaphore> mSemaphores;
        std::vector<vk::SemaphoreSubmitInfoKHR> mInfos;

    public:
        void add(VulkanUniqueSemaphore semaphore, GpuPipelineStage stage);
    };

    class CommandBufferList
    {
        friend class VulkanGpuDevice;

        std::vector<VulkanUniqueCommandBuffer> mCommandBuffers;
        std::vector<vk::CommandBufferSubmitInfoKHR> mInfos;

    public:
        void add(VulkanCommandListGraphics cmd_list);
    };

private:
    // core device objects

    // keeps the connection to vulkan dynamic library
    vk::DynamicLoader mLoader;
    vk::DispatchLoaderDynamic mDispatch;
    VulkanUniqueInstance mInstance;
    VulkanUniqueDebugMessenger mDebugUtilsMessenger;
    vk::PhysicalDevice mPhysicalDevice;
    VulkanUniqueDevice mDevice;

    // initialization

    static uint32_t select_queue(
        std::vector<vk::QueueFamilyProperties>& queue_family,
        const vk::QueueFlags& queue_flags);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback_dispatcher(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    VkBool32 debug_messenger_callback(
        const vk::DebugUtilsMessageSeverityFlagsEXT& message_severity,
        const vk::DebugUtilsMessageTypeFlagsEXT& message_type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callback_data) const;

    void create_instance();
    void create_debug_report();
    void select_physical_device();
    void create_device_and_queues();

    // graphics queue & submission

    vk::Queue mGraphicsQueue;
    std::uint32_t mGraphicsQueueFamilyIndex = -1;

    // swapchain & presentation

    void check_queue_presentation_capacity_throw(
        uint32_t queue_family_index) const;
    static const char * platform_surface_extension_name();

    std::map<NativeWindow *, std::unique_ptr<VulkanSwapchain>> mSwapchainCache;
    // implemented in platform-specific WSI integration libs
    std::unique_ptr<VulkanSwapchain> & create_swapchain(NativeWindow *window);

    // thread resources

    std::vector<VulkanUniqueCommandPool> mCommandPools;

    // frame resources

    std::vector<VulkanUniqueSemaphore> mSemaphoreAvail;
    std::vector<VulkanUniqueFence> mFenceAvail;

    void release_semaphore(VulkanUniqueSemaphore semaphore);
    void release_fence(VulkanUniqueFence fence);

    struct BatchResources
    {
        CommandBufferList cmd_lists;
        SemaphoreInfo wait_semaphores, signal_semaphores;
        VulkanUniqueFence fence;
    };
    std::deque<BatchResources> mOutstandingBatches;

    void release_resources(BatchResources &batch);

public:
    VulkanGpuDevice();
    ~VulkanGpuDevice();

    // determines the number of command allocators, etc according to the
    // number of threads used in compiling the command lists. since services
    // can't have dependencies, this information must be injected.
    void set_thread_resource_pool_size(std::size_t num_threads);

    // todo ensure correct concurrent behavior
    VulkanCommandListGraphics allocate_graphics_command_list(
        std::size_t thread_id);

    static auto create_command_buffer_list() { return CommandBufferList(); }
    static auto create_semaphore_info() { return SemaphoreInfo(); }

    void submit_graphics_jobs(
        CommandBufferList cmd_lists,
        SemaphoreInfo wait_semaphores,
        SemaphoreInfo signal_semaphores
    );

    // check for finished frames and free associated resources
    void reclaim_resources();

    // synchronization primitives allocated from here are either freed
    // when unused, or passed back as part of the submission. same as the
    // command buffers.

    VulkanUniqueSemaphore allocate_semaphore();
    VulkanUniqueFence allocate_fence();

    VulkanSwapchain & swapchain(NativeWindow *window);
    void destroy_swapchain(NativeWindow *window);

    vk::Device device() const { return mDevice.get(); }
    vk::PhysicalDevice physical_device() const { return mPhysicalDevice; }

    // todo these functions should not be accessed by game systems

private:
    auto create_shader_module(const auto &create_info)
    {
        return device().createShaderModuleUnique(
            create_info, nullptr, dispatch());
    }

    auto create_descriptor_set_layout(const auto &create_info)
    {
        return device().createDescriptorSetLayoutUnique(
            create_info, nullptr, dispatch());
    }

    auto create_pipeline_layout(const auto &create_info)
    {
        return device().createPipelineLayoutUnique(
            create_info, nullptr, dispatch());
    }

    auto create_render_pass(const auto &create_info)
    {
        return device().createRenderPassUnique(
            create_info, nullptr, dispatch());
    }

    auto create_graphics_pipeline(const auto &create_info)
    {
        auto [result, value] = device().createGraphicsPipelineUnique(
            { }, create_info, nullptr, dispatch());
        return std::pair { std::move(value), result };
    }

    auto create_swapchain(const auto &create_info)
    {
        return device().createSwapchainKHRUnique(
            create_info, nullptr, dispatch());
    }

    vk::Queue present_queue() const { return mGraphicsQueue; }
    vk::Queue graphics_queue() const { return mGraphicsQueue; }

    const vk::DispatchLoaderDynamic & dispatch() const { return mDispatch; }
};
}
