#pragma once

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Time/ScheduleNodeTimePointReady.hpp>

namespace usagi
{
/**
 * \brief The root node of the schedule constraint graph.
 */
struct ScheduleNodeRoot
    : ScheduleNodeTimePointReady
{
};
}
