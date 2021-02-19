#include "Clock.hpp"

// #include "Logging.hpp"

usagi::Clock::Clock()
{
    // if constexpr(!ClockT::is_steady)
        // LOG(warn, "std::chrono::high_resolution_clock is not steady!");
    reset();
}

void usagi::Clock::reset()
{
    mCreation = mLastTick = ClockT::now();
    mTotalFrameTime = mLastFrameTime = mLastTick - mCreation;
}

usagi::TimeDuration usagi::Clock::realtime_elapsed() const
{
    const Duration d = ClockT::now() - mLastTick;
    return d.count();
}

usagi::TimeDuration usagi::Clock::realtime_total_elapsed() const
{
    const Duration d = ClockT::now() - mCreation;
    return d.count();
}

usagi::TimeDuration usagi::Clock::tick()
{
    const auto this_tick = ClockT::now();
    mLastFrameTime = this_tick - mLastTick;

    using namespace std::chrono_literals;
    // if(mSinceLastTick > 1s)
    // {
    //     mSinceLastTick = 16ms;
    //     LOG(warn, "Tick time > 1s. Set to 16ms.");
    // }
    mLastTick = this_tick;
    mTotalFrameTime = this_tick - mCreation;
    return last_frame_time();
}

usagi::TimeDuration usagi::Clock::last_frame_time() const
{
    return mLastFrameTime.count();
}

usagi::TimeDuration usagi::Clock::total_frame_time() const
{
    return mTotalFrameTime.count();
}
