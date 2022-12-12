#pragma once

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Time/ScheduleNodeTimeSpan.hpp>

namespace usagi
{
template <typename TaskIndexT>
struct ScheduleNodeExecuteTask : ScheduleNodeTimeSpan
{
    TaskIndexT task_id = -1;
    float exec_time = NAN;

    void derive_finish_time()
    {
        finish_time = ready_time + exec_time;
    }
};
}
