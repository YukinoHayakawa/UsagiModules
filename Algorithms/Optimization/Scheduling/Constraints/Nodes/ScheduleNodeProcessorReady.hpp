#pragma once

#include <cstdint>

#include "ScheduleNodeTimePoint.hpp"

namespace usagi
{
struct ScheduleNodeProcessorReady : ScheduleNodeTimePoint
{
    std::uint8_t proc_id;
    // ... next node
};
}
