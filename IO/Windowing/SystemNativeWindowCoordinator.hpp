#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Modules/Runtime/Executive/ServiceStateTransitionGraph.hpp>
#include <Usagi/Modules/Common/Math/ComponentRegion2D.hpp>
#include <Usagi/Runtime/Service/Service.hpp>

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
        auto &tg = USAGI_SERVICE(rt, ServiceStateTransitionGraph);

        for(auto &&e : db.view(C<ComponentNativeWindow>()))
        {
            auto &c_wnd = USAGI_COMPONENT(e, ComponentNativeWindow);
            auto &c_region = USAGI_COMPONENT(e, ComponentRegion2D);

            const auto id = c_wnd.identifier.to_string_view();
            // create the window if a corresponding window can't be found
            // for the entity
            if(const auto h_wnd = wnd_mgr.window(id); !h_wnd)
            {
                // todo: restore window state (maximized/etc)
                wnd_mgr.create_window(
                    id,
                    "test",
                    c_region.position,
                    c_region.size,
                    c_wnd.dpi_scaling,
                    c_wnd.state
                );
            }
            else if(h_wnd->closed())
            {
                using Action = ComponentNativeWindow::OnCloseAction;

                switch(c_wnd.on_close)
                {
                    // todo:
                    case Action::DESTROY_ENTITY:
                        e.destroy();
                        break;
                    // if the window is used to notify the exit of the program,
                    // don't destroy it. otherwise the program will lose the
                    // main window.
                    case Action::NOTIFY_EXIT:
                        tg.should_exit = true;
                        break;
                    default: USAGI_INVALID_ENUM_VALUE();
                }
            }
            // synchronize the window state
            else
            {
                c_region.position = h_wnd->position();
                c_region.size = h_wnd->logical_size();
                c_wnd.dpi_scaling = h_wnd->dpi_scaling();
                c_wnd.state = h_wnd->state();
            }
        }

        wnd_mgr.destroy_unused_windows();
    }
};
}
