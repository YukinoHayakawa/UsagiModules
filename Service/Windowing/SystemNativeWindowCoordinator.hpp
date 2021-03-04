#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Module/Common/Math/ComponentRegion2D.hpp>
#include <Usagi/Runtime/Service.hpp>

#include "ComponentNativeWindow.hpp"
#include "NativeWindow.hpp"
#include "ServiceNativeWindowManager.hpp"

namespace usagi
{
struct SystemNativeWindowCoordinator
{
    using WriteAccess = C<ComponentNativeWindow, ComponentRegion2D>;
    using ReadAccess = C<>;

    void update(auto &&rt, auto &&db)
    {
        auto &wnd_mgr = USAGI_SERVICE(rt, ServiceNativeWindowManager);

        for(auto &&e : db.view(C<ComponentNativeWindow>()))
        {
            auto &c_wnd = USAGI_COMPONENT(e, ComponentNativeWindow);
            auto &c_region = USAGI_COMPONENT(e, ComponentRegion2D);

            const auto id = c_wnd.identifier.str();
            const auto h_wnd = wnd_mgr.window(id);
            if(!h_wnd)
            {
                wnd_mgr.create_window(
                    id,
                    "test",
                    c_region.position,
                    c_region.size
                );
            }
            else if(h_wnd->closed())
            {
                e.destroy();
            }
            else
            {
                c_region.position = h_wnd->position();
                c_region.size = h_wnd->size();
                c_wnd.dpi_scaling = h_wnd->dpi_scaling();
            }
        }

        wnd_mgr.destroy_unused_windows();
    }
};
}
