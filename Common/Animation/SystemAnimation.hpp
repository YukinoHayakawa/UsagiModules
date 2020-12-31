#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Module/Common/Math/Lerp.hpp>
#include <Usagi/Module/Common/Time/ServiceMasterClock.hpp>
#include <Usagi/Runtime/Service.hpp>

#include "ComponentAnimator.hpp"

namespace usagi
{
struct SystemAnimation
{
    using WriteAccess = C<ComponentAnimator>;
    using ReadAccess = C<TagAnimatorLoop>;

    // todo impl looping animations
    auto update(auto &&rt, auto &&db)
    {
        auto &clock = USAGI_SERVICE(rt, ServiceMasterClock);
        const auto current_time = clock.frame_begin_time();

        for(auto &&e : db.view(
            C<ComponentAnimator>()
        ))
        {
            auto &ani = USAGI_COMPONENT(e, ComponentAnimator);

            if(ani.start_time == ComponentAnimator::USE_CURRENT_TIME)
                ani.start_time = current_time;

            assert(ani.duration > 0);

            if(ani.start_time > current_time) continue;
            if(ani.finished())
            {
                // warning: finished animator not removed
                continue;
            }

            const auto x = inverse_lerp(
                0.0,
                ani.duration,
                current_time - ani.start_time
            );

            assert(ani.func == TimingFunction::LINEAR);

            // linear interpolation function
            ani.y = std::clamp(static_cast<float>(x), 0.f, 1.f);
        }
    }
};
}
