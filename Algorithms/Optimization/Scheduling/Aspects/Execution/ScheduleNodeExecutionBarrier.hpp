#pragma once

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Time/ScheduleNodeTimeSpan.hpp>

namespace usagi
{
template <typename TaskIndexT>
struct ScheduleNodeExecutionBarrier
    : ScheduleNodeTimeSpan
{
    TaskIndexT num_waiting_inputs = 0;
};
}
