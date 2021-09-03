#include <Usagi/Library/Utility/Pointer.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/Platforms/WinCommon/Windowing/NativeWindowWin32.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>

namespace usagi
{
const char * VulkanGpuDevice::platform_surface_extension_name()
{
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

void VulkanGpuDevice::check_queue_presentation_capacity_throw(
    const uint32_t queue_family_index) const
{
    if(!mPhysicalDevice.getWin32PresentationSupportKHR(
        queue_family_index,
        mDispatch
    )) USAGI_THROW(
        std::runtime_error(
            "Graphics queue doesn't support Win32 presentation")
    );
}

std::unique_ptr<VulkanSwapchain> &
VulkanGpuDevice::create_swapchain(NativeWindow *window)
{
    auto &win32_window = dynamic_cast_ref_throw<NativeWindowWin32>(window);

    // todo
    // LOG(info, "Creating Win32Surface for window: {}", win32_window.title());

    vk::Win32SurfaceCreateInfoKHR surface_create_info;
    surface_create_info.setHinstance(
        reinterpret_cast<HINSTANCE>(
            GetWindowLongPtrW(win32_window.handle(), GWLP_HINSTANCE))
    );
    surface_create_info.setHwnd(win32_window.handle());

    auto surface = mInstance->createWin32SurfaceKHRUnique(
        surface_create_info,
        nullptr,
        mDispatch
    );
    if(!mPhysicalDevice.getSurfaceSupportKHR(
        mGraphicsQueueFamilyIndex,
        surface.get(),
        mDispatch
    )) USAGI_THROW(
        std::runtime_error(
            "Graphics queue doesn't support Win32 surface")
    );

    auto swapchain = std::make_unique<VulkanSwapchain>(
        this,
        std::move(surface)
    );
    swapchain->create(
        win32_window.surface_size().cast<std::uint32_t>(),
        vk::Format::eB8G8R8A8Unorm
    );

    const auto [fst, snd] = mSwapchainCache.try_emplace(
        window,
        std::move(swapchain)
    );
    assert(snd);

    return fst->second;
}
}
