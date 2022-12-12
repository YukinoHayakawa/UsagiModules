#pragma once

#include <limits>

namespace usagi
{
/**
 * \brief Time point that signals something can start.
 */
struct ScheduleNodeTimePointReady
{
    float ready_time = std::numeric_limits<float>::lowest();

    // take the later time as the new ready time.
    float update_ready_time(const float another)
    {
        return ready_time = std::max(another, ready_time);
    }

    bool operator==(const ScheduleNodeTimePointReady &rhs) const = default;
};
}
