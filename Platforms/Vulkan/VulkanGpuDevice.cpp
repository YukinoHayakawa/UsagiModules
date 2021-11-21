#include "VulkanGpuDevice.hpp"

#include <Usagi/Library/Algorithm/Container.hpp>
#include <Usagi/Library/Utility/BitMask.hpp>
#include <Usagi/Library/Utility/String.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
VulkanGpuDevice::VulkanGpuDevice()
{
    create_instance();
    create_debug_report();
    select_physical_device();
    create_device_and_queues();
}

VulkanGpuDevice::~VulkanGpuDevice()
{
    assert(mSwapchainCache.empty());
}

VkBool32 VulkanGpuDevice::debug_messenger_callback_dispatcher(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    const auto device = reinterpret_cast<VulkanGpuDevice*>(user_data);
    return device->debug_messenger_callback(
        static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity),
        static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type),
        reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT *>(
            callback_data));
}

VkBool32 VulkanGpuDevice::debug_messenger_callback(
    const vk::DebugUtilsMessageSeverityFlagsEXT &message_severity,
    const vk::DebugUtilsMessageTypeFlagsEXT &message_type,
    const vk::DebugUtilsMessengerCallbackDataEXT *callback_data) const
{
    using namespace logging;

    // see https://www.lunarg.com/wp-content/uploads/2018/05/Vulkan-Debug-Utils_05_18_v1.pdf for example code

    LoggingLevel level;
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    // using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
    // bool validation_error = false;

    if(message_severity & Severity::eVerbose)
        level = LoggingLevel::debug;
    else if(message_severity & Severity::eInfo)
        level = LoggingLevel::info;
    else if(message_severity & Severity::eWarning)
        level = LoggingLevel::warn;
    else if(message_severity & Severity::eError)
        level = LoggingLevel::error;
    else
        level = LoggingLevel::info;

    log(level,
        "[Vulkan] {} : {} - Message ID Number {}, Message ID String {}:\n{}",
        to_string(message_severity),
        to_string(message_type),
        callback_data->messageIdNumber,
        USAGI_OPT_STRING(callback_data->pMessageIdName),
        callback_data->pMessage);

    if(level == LoggingLevel::error)
    {
        // todo exception::printStacktrace();
    }

    if(callback_data->objectCount > 0)
    {
        log(level, "    Objects - {}",
            callback_data->objectCount);
        for(uint32_t i = 0; i < callback_data->objectCount; ++i)
        {
            const auto &object = callback_data->pObjects[i];
            log(level, "        Object[{}] - Type {}, Value {}, Name \"{}\"",
                i,
                to_string(object.objectType),
                object.objectHandle,
                USAGI_OPT_STRING(object.pObjectName)
            );
        }
    }
    if(callback_data->cmdBufLabelCount > 0)
    {
        log(level, "    Command Buffer Label - {}",
            callback_data->cmdBufLabelCount);
        for(uint32_t i = 0; i < callback_data->cmdBufLabelCount; ++i)
        {
            log(level,
                "        Label[{}] - {} {{ {}, {}, {}, {} }}\n",
                i,
                callback_data->pCmdBufLabels[i].pLabelName,
                callback_data->pCmdBufLabels[i].color[0],
                callback_data->pCmdBufLabels[i].color[1],
                callback_data->pCmdBufLabels[i].color[2],
                callback_data->pCmdBufLabels[i].color[3]);
        }
    }

    // if(message_type & Type::eValidation) {
    //     validation_error = true;
    // }

    // Don't bail out, but keep going. return false;
    return false;
}

uint32_t VulkanGpuDevice::select_queue(
    std::vector<vk::QueueFamilyProperties> &queue_family,
    const vk::QueueFlags &queue_flags)
{
    for(auto iter = queue_family.begin(); iter != queue_family.end(); ++iter)
        if(match_bit_mask(iter->queueFlags, queue_flags))
            return static_cast<uint32_t>(iter - queue_family.begin());
    USAGI_THROW(std::runtime_error(
        "Could not find a queue family with required flags."));
}

void VulkanGpuDevice::create_instance()
{
    const auto instance_proc =
        mLoader.getProcAddress<PFN_vkGetInstanceProcAddr>(
            "vkGetInstanceProcAddr"
        );
    mDispatch.init(instance_proc);

    LOG(info, "Creating Vulkan instance");
    LOG(info, "--------------------------------");

    vk::ApplicationInfo application_info;
    application_info.setPApplicationName("UsagiEngine");
    application_info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    application_info.setPEngineName("UsagiEngine");
    application_info.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    application_info.setApiVersion(VK_API_VERSION_1_0);

    // Extensions
    {
        LOG(info, "Available instance extensions");
        LOG(info, "--------------------------------");
        for(auto &&ext : enumerateInstanceExtensionProperties(
            nullptr,
            mDispatch
        )) LOG(info, ext.extensionName.data());
        LOG(info, "--------------------------------");
    }
    // Validation layers
    {
        LOG(info, "Available instance layers");
        LOG(info, "--------------------------------");
        for(auto &&layer : enumerateInstanceLayerProperties(mDispatch))
        {
            LOG(info, "Name       : {}", layer.layerName);
            LOG(info, "Description: {}", layer.description);
            LOG(info, "--------------------------------");
        }
    }

    vk::InstanceCreateInfo instance_create_info;
    instance_create_info.setPApplicationInfo(&application_info);

    const std::array instance_extensions =
    {
        // application window
        VK_KHR_SURFACE_EXTENSION_NAME,
        // provide feedback from validation layer, etc.
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        platform_surface_extension_name(),
        // required by VK_KHR_synchronization2
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
    instance_create_info.setEnabledExtensionCount(
        static_cast<uint32_t>(instance_extensions.size()));
    instance_create_info.setPpEnabledExtensionNames(instance_extensions.data());

    // Enabled Extensions
    {
        LOG(info, "Enabled instance extensions");
        LOG(info, "--------------------------------");
        for(auto &&ext : instance_extensions)
            LOG(info, ext);
    }

    // todo: enumerate layers first
    // todo: disable in release
    const std::array<const char *, 1> layers =
    {
#ifndef _NDEBUG
        "VK_LAYER_KHRONOS_validation"
#endif
    };
    instance_create_info.setEnabledLayerCount(
        static_cast<uint32_t>(layers.size()));
    instance_create_info.setPpEnabledLayerNames(layers.data());

    LOG(info, "--------------------------------");

    // Enabled Validation layers
    {
        LOG(info, "Enabled instance layers");
        LOG(info, "--------------------------------");
        for(auto &&layer : layers)
            LOG(info, layer);
    }

    LOG(info, "--------------------------------");

    mInstance = createInstanceUnique(
        instance_create_info,
        nullptr,
        mDispatch
    );

    // further initialize instance-related functions including
    // vkDestroyInstance
    mDispatch.init(mInstance.get(), instance_proc);

    LOG(info, "Vulkan instance created.");
}

void VulkanGpuDevice::create_debug_report()
{
    vk::DebugUtilsMessengerCreateInfoEXT info;
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    info.messageSeverity =
        // Severity::eVerbose |
        Severity::eInfo |
        Severity::eWarning |
        Severity::eError;
    using Type = vk::DebugUtilsMessageTypeFlagBitsEXT;
    info.messageType =
        Type::eGeneral |
        Type::eValidation |
        Type::ePerformance;
    info.pfnUserCallback = &debug_messenger_callback_dispatcher;

    mDebugUtilsMessenger = mInstance->createDebugUtilsMessengerEXTUnique(
        info, nullptr, mDispatch
    );
}

void VulkanGpuDevice::select_physical_device()
{
    LOG(info, "--------------------------------");
    LOG(info, "Available physical devices");
    LOG(info, "--------------------------------");
    for(auto physical_devices = mInstance->enumeratePhysicalDevices(mDispatch);
        auto &&dev : physical_devices)
    {
        const auto prop = dev.getProperties(mDispatch);
        LOG(info, "Device Name   : {}", prop.deviceName);
        LOG(info, "Device Type   : {}", to_string(prop.deviceType));
        LOG(info, "Device ID     : {}", prop.deviceID);
        LOG(info, "API Version   : {}", prop.apiVersion);
        LOG(info, "Driver Version: {}", prop.vendorID);
        LOG(info, "Vendor ID     : {}", prop.vendorID);
        LOG(info, "--------------------------------");
        // todo: select device based on features and score them / let the
        // user choose
        if(!mPhysicalDevice)
            mPhysicalDevice = dev;
        else if(prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            mPhysicalDevice = dev;
    }

    if(!mPhysicalDevice)
        USAGI_THROW(std::runtime_error("No available GPU supporting Vulkan."));
    LOG(info, "Using physical device: {}",
        mPhysicalDevice.getProperties(mDispatch).deviceName);
}

void VulkanGpuDevice::create_device_and_queues()
{
    LOG(info, "Creating device and queues");

    auto queue_families = mPhysicalDevice.getQueueFamilyProperties(mDispatch);
    LOG(info, "Supported queue families:");
    for(std::size_t i = 0; i < queue_families.size(); ++i)
    {
        auto &qf = queue_families[i];
        LOG(info, "#{}: {} * {}", i, to_string(qf.queueFlags), qf.queueCount);
    }

    const auto graphics_queue_index = select_queue(queue_families,
        vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer);
    check_queue_presentation_capacity_throw(graphics_queue_index);

    LOG(info, "Getting a queue from queue family {}.",
        graphics_queue_index);

    vk::DeviceCreateInfo device_create_info;

    vk::PhysicalDeviceFeatures features;
    device_create_info.setPEnabledFeatures(&features);
    features.setFillModeNonSolid(true);
    features.setLargePoints(true);
    features.setWideLines(true);

    vk::DeviceQueueCreateInfo queue_create_info[1];
    float queue_priority = 1;
    queue_create_info[0].setQueueFamilyIndex(graphics_queue_index);
    queue_create_info[0].setQueueCount(1);
    queue_create_info[0].setPQueuePriorities(&queue_priority);
    device_create_info.setQueueCreateInfoCount(1);
    device_create_info.setPQueueCreateInfos(queue_create_info);

    // todo: check device capacity
    // todo: move to instance ext
    const std::vector device_extensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };
    device_create_info.setEnabledExtensionCount(static_cast<uint32_t>(
        device_extensions.size()));
    device_create_info.setPpEnabledExtensionNames(device_extensions.data());

    mDevice = mPhysicalDevice.createDeviceUnique(
        device_create_info, nullptr, mDispatch);
    mDispatch.init(mDevice.get());

    mGraphicsQueue = mDevice->getQueue(
        graphics_queue_index,
        0,
        mDispatch
    );
    mGraphicsQueueFamilyIndex = graphics_queue_index;
}

void VulkanGpuDevice::set_thread_resource_pool_size(std::size_t num_threads)
{
    resize_generate(mCommandPools, num_threads, [this]() {
        vk::CommandPoolCreateInfo info;
        info.setQueueFamilyIndex(mGraphicsQueueFamilyIndex);
        return mDevice->createCommandPoolUnique(info, nullptr, mDispatch);
    });
}

VulkanCommandListGraphics VulkanGpuDevice::allocate_graphics_command_list(
    const std::size_t thread_id)
{
    assert(thread_id <= mCommandPools.size());

    vk::CommandBufferAllocateInfo info;

    info.setCommandBufferCount(1);
    info.setCommandPool(mCommandPools[thread_id].get());
    info.setLevel(vk::CommandBufferLevel::ePrimary);

    return VulkanCommandListGraphics(
        this,
        std::move(mDevice->allocateCommandBuffersUnique(
            info, mDispatch
        ).front())
    );
}

void VulkanGpuDevice::SemaphoreInfo::add(
    VulkanUniqueSemaphore semaphore,
    GpuPipelineStage stage)
{
    vk::SemaphoreSubmitInfoKHR info;

    info.semaphore = semaphore.get();
    info.value = 1; // might be other values with timeline semaphores
    info.stageMask = Vulkan_GpuPipelineStage(stage);
    info.deviceIndex = 0;

    mInfos.emplace_back(info);
    mSemaphores.emplace_back(std::move(semaphore));
}

void VulkanGpuDevice::CommandBufferList::add(
    VulkanCommandListGraphics cmd_list)
{
    auto &info = mInfos.emplace_back();
    info.commandBuffer = cmd_list.mCommandBuffer.get();
    info.deviceMask = 0;
    mCommandBuffers.emplace_back(std::move(cmd_list.mCommandBuffer));
}

// https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/extensions/VK_KHR_synchronization2.md
void VulkanGpuDevice::submit_graphics_jobs(
    CommandBufferList cmd_lists,
    SemaphoreInfo wait_semaphores,
    SemaphoreInfo signal_semaphores)
{
    // todo support multiple queues

    BatchResources &res = mOutstandingBatches.emplace_back();

    res.cmd_lists = std::move(cmd_lists);
    res.wait_semaphores = std::move(wait_semaphores);
    res.signal_semaphores = std::move(signal_semaphores);
    res.fence = allocate_fence();

    vk::SubmitInfo2KHR submit;

    submit.setCommandBufferInfos(res.cmd_lists.mInfos);
    submit.setWaitSemaphoreInfos(res.wait_semaphores.mInfos);
    submit.setSignalSemaphoreInfos(res.signal_semaphores.mInfos);

    mGraphicsQueue.submit2KHR({ submit }, res.fence.get(), mDispatch);
}

void VulkanGpuDevice::reclaim_resources()
{
    for(auto ii = mOutstandingBatches.begin();
        ii != mOutstandingBatches.end();)
    {
        if(mDevice->getFenceStatus(ii->fence.get(), mDispatch)
            == vk::Result::eSuccess)
        {
            release_resources(*ii);
            ii = mOutstandingBatches.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
}

void VulkanGpuDevice::release_semaphore(VulkanUniqueSemaphore semaphore)
{
    mSemaphoreAvail.push_back(std::move(semaphore));
}

void VulkanGpuDevice::release_fence(VulkanUniqueFence fence)
{
    std::array fences { fence.get() };
    mDevice->resetFences(fences, mDispatch);
    mFenceAvail.push_back(std::move(fence));
}

void VulkanGpuDevice::release_resources(BatchResources &batch)
{
    release_fence(std::move(batch.fence));
    for(auto &&s : batch.wait_semaphores.mSemaphores)
        release_semaphore(std::move(s));
    for(auto &&s : batch.signal_semaphores.mSemaphores)
        release_semaphore(std::move(s));
}

VulkanUniqueSemaphore VulkanGpuDevice::allocate_semaphore()
{
    return pop_or_create(mSemaphoreAvail, [this]() {
        return mDevice->createSemaphoreUnique({ }, nullptr, mDispatch);
    });
}

VulkanUniqueFence VulkanGpuDevice::allocate_fence()
{
    return pop_or_create(mFenceAvail, [this]() {
        return mDevice->createFenceUnique({ }, nullptr, mDispatch);
    });
}

VulkanSwapchain & VulkanGpuDevice::swapchain(NativeWindow * window)
{
    auto *swapchain = find_or_create(
        mSwapchainCache,
        window,
        [this](auto&& wnd) -> auto & { return create_swapchain(wnd); }
    ).get();
    return *swapchain;
}

void VulkanGpuDevice::destroy_swapchain(NativeWindow *window)
{
    mSwapchainCache.erase(window);
}
}
