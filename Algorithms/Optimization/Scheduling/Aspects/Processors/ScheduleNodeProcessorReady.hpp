#pragma once

#include <cassert>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Time/ScheduleNodeTimePointReady.hpp>

namespace usagi
{
/**
 * \brief A processor ready node represents a processor available for executing
 * a task.
 * \tparam ProcIndexT Type of processor index.
 */
// template <typename ProcIndexT>
struct ScheduleNodeProcessorReady
    : ScheduleNodeTimePointReady
{
    std::uint8_t processor_index = -1;

    // if the processor is occupied it cannot be assigned to execute tasks.
    bool occupied = false;

    bool operator==(const ScheduleNodeProcessorReady &rhs) const = default;

    std::partial_ordering operator<=>(
        const ScheduleNodeProcessorReady &rhs) const
    {
        // todo: occupied state should not participate in comparisons.
        if(occupied && !rhs.occupied)
            return std::partial_ordering::greater;
        if(!occupied && rhs.occupied)
            return std::partial_ordering::less;
        return ready_time <=> rhs.ready_time;
    }

    void occupy()
    {
        assert(!occupied);
        occupied = true;
    }
};
}
