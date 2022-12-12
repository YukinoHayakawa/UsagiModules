#pragma once

#include <limits>

namespace usagi
{
/**
 * \brief Time point that signals something is done.
 */
struct ScheduleNodeTimePointFinish
{
    float finish_time = std::numeric_limits<float>::lowest();
};
}
