#pragma once

#include <chrono>

namespace usagi
{
using TimeDuration = double;
using TimePoint = double;

class Clock
{
    using ClockT = std::chrono::high_resolution_clock;
    ClockT::time_point mCreation;
    ClockT::time_point mLastTick;
    using Duration = std::chrono::duration<double>;
    Duration mLastFrameTime;
    Duration mTotalFrameTime;

public:
    Clock();

    void reset();

    // Calculate elapsed time since last tick and update tick.
    TimeDuration tick();


    // Time in second from last tick.
    TimeDuration realtime_elapsed() const;

    // Time in second since clock creation/last reset.
    TimeDuration realtime_total_elapsed() const;


    // Time between last two ticks
    TimeDuration last_frame_time() const;

    // Time since start to last tick.
    TimeDuration total_frame_time() const;
};
}
