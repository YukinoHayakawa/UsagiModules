#pragma once

#include "ScheduleNodeTimePointFinish.hpp"
#include "ScheduleNodeTimePointReady.hpp"

namespace usagi
{
struct ScheduleNodeTimeSpan
    : ScheduleNodeTimePointReady
    , ScheduleNodeTimePointFinish
{
};
}
