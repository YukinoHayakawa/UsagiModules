#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Module/Common/Executive/ServiceTransitionGraph.hpp>
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
        auto &tg = USAGI_SERVICE(rt, ServiceTransitionGraph);

        for(auto &&e : db.view(C<ComponentNativeWindow>()))
        {
            auto &c_wnd = USAGI_COMPONENT(e, ComponentNativeWindow);
            auto &c_region = USAGI_COMPONENT(e, ComponentRegion2D);

            const auto id = c_wnd.identifier.str();
            const auto h_wnd = wnd_mgr.window(id);
            // create the window if a corresponding window can't be found
            // for the entity
            if(!h_wnd)
            {
                wnd_mgr.create_window(
                    id,
                    "test",
                    c_region.position,
                    c_region.size
                );
            }
            else if(h_wnd->should_close())
            {
                using Action = ComponentNativeWindow::OnCloseAction;

                switch(c_wnd.on_close)
                {
                    case Action::DESTROY_ENTITY:
                        e.destroy();
                        break;
                    case Action::NOTIFY_EXIT:
                        tg.should_exit = true;
                        break;
                    default: USAGI_UNREACHABLE();
                }
            }
            // synchronize the window state
            // todo: sync in two directions
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
