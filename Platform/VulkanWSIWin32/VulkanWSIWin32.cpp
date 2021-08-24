#include <Usagi/Library/Utility/Pointer.hpp>
#include <Usagi/Module/Common/Logging/Logging.hpp>
#define VK_USE_PLATFORM_WIN32_KHR
#include <Usagi/Module/Platform/Vulkan/VulkanGpuDevice.hpp>
#include <Usagi/Module/Platform/WinCommon/Windowing/NativeWindowWin32.hpp>

namespace usagi
{
const char * VulkanGpuDevice::platform_surface_extension_name()
{
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

void VulkanGpuDevice::check_queue_presentation_capacity_throw(
    const uint32_t queue_family_index) const
{
    if (!mPhysicalDevice.getWin32PresentationSupportKHR(queue_family_index))
        USAGI_THROW(std::runtime_error(
            "Graphics queue doesn't support Win32 presentation"));
}

VulkanSwapchain & VulkanGpuDevice::create_swapchain(NativeWindow* window)
{
    auto &win32_window = dynamic_cast_ref_throw<NativeWindowWin32>(window);

    // todo
    // LOG(info, "Creating Win32Surface for window: {}", win32_window.title());

    vk::Win32SurfaceCreateInfoKHR surface_create_info;
    surface_create_info.setHinstance(reinterpret_cast<HINSTANCE>(
        GetWindowLongPtrW(win32_window.handle(), GWLP_HINSTANCE)));
    surface_create_info.setHwnd(win32_window.handle());

    auto surface = mInstance->createWin32SurfaceKHRUnique(surface_create_info);
    if (!mPhysicalDevice.getSurfaceSupportKHR(
        mGraphicsQueueFamilyIndex, surface.get()
    )) USAGI_THROW(std::runtime_error(
        "Graphics queue doesn't support Win32 surface"));

    auto swapchain = std::make_unique<VulkanSwapchain>(
        this, std::move(surface)
    );

    const auto [fst, snd] = mSwapchainCache.try_emplace(
        window, std::move(swapchain)
    );
    assert(snd);

    return *fst->second.get();
}
}
