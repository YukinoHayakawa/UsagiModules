#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service.hpp>

#include "ComponentAnimator.hpp"

namespace usagi
{
struct SystemRemoveFinishedAnimation
{
    using WriteAccess = C<ComponentAnimator>;
    using ReadAccess = C<TagAnimatorLoop>;

    auto update(auto &&rt, auto &&db)
    {
        for(auto &&e : db.view(
            C<ComponentAnimator>(),
            C<TagAnimatorLoop>()
        ))
        {
            auto &ani = USAGI_COMPONENT(e, ComponentAnimator);

            if(ani.finished())
                e.remove_component(ComponentAnimator());
        }
    }
};
}
