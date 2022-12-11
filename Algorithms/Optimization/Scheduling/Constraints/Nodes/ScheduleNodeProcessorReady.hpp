#pragma once

#include <cstdint>

namespace usagi
{
struct ScheduleNodeProcessorReady
{
    std::uint8_t proc_id;
    float ready_time;
    bool occupied = false;
    // ... next node
    //
    // bool operator<(const ScheduleNodeProcessorReady &rhs) const
    // {
    //     if(occupied && !rhs.occupied)
    //         return false;
    //     if(!occupied && rhs.occupied)
    //         return true;
    //     return ready_time < rhs.ready_time;
    // }

    bool operator==(const ScheduleNodeProcessorReady &rhs) const = default;

    std::partial_ordering operator<=>(const ScheduleNodeProcessorReady &rhs) const
    {
        if(occupied && !rhs.occupied)
            return std::partial_ordering::greater;
        if(!occupied && rhs.occupied)
            return std::partial_ordering::less;
        return ready_time <=> rhs.ready_time;
    }
};

static_assert(std::totally_ordered_with<ScheduleNodeProcessorReady, ScheduleNodeProcessorReady>);
}
