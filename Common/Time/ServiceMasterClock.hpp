#pragma once

#include "Clock.hpp"

namespace usagi
{
struct ServiceMasterClock
{
    using ServiceType = Clock;

    Clock master_clock;

    ServiceType & get_service()
    {
        return master_clock;
    }
};
}
