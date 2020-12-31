#pragma once

#include <cstdint>

#include <Usagi/Entity/Component.hpp>

namespace usagi
{
enum class TimingFunction : std::uint8_t
{
    LINEAR,

    SIZE
};

struct ComponentAnimator
{
    static constexpr double USE_CURRENT_TIME = -1;

    double start_time = USE_CURRENT_TIME;
    double duration = 1;
    // interpolated value in [0, 1]
    float y = 0;
    // interpolation function
    TimingFunction func = TimingFunction::LINEAR;
    std::uint8_t padding[3];

    bool finished() const
    {
        return y == 1;
    }
};

// todo not implemented
struct ComponentAnimatorStat
{
    std::uint32_t iteration = 0;
};

// Update animator.
// USAGI_DECL_TAG_COMPONENT(TagAnimatorActivated);
// Loop the animator.
USAGI_DECL_TAG_COMPONENT(TagAnimatorLoop);
// Remove the component when the animation is finished.
USAGI_DECL_TAG_COMPONENT(TagAnimatorDisposable);
}
