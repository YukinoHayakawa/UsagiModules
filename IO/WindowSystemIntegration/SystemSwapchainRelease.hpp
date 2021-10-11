#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Modules/Services/Windowing/ComponentNativeWindow.hpp>
#include <Usagi/Modules/Services/Windowing/ServiceNativeWindowManager.hpp>

// todo remove
#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>

namespace usagi
{
USAGI_DECL_ALIAS_SERVICE(
    ServiceHardwareGraphics,
    LazyInitService<VulkanGpuDevice>
);

struct SystemSwapchainRelease
{
    using WriteAccess = C<>;
    using ReadAccess = C<ComponentNativeWindow, TagNativeWindowDestroy>;

    void update(auto &&rt, auto &&db)
    {
        for(auto &&e : db.view(ReadAccess()))
        {
            auto& wnd_mgr = USAGI_SERVICE(rt, ServiceNativeWindowManager);
            auto& gfx = USAGI_SERVICE(rt, ServiceHardwareGraphics);

            auto& c_wnd = USAGI_COMPONENT(e, ComponentNativeWindow);

            // if the window is about to be destroyed, destroy the associated
            // swapchain first
            NativeWindow* const wnd = wnd_mgr.window(c_wnd.identifier.str());
            gfx.destroy_swapchain(wnd);
        }
    }
};
}
