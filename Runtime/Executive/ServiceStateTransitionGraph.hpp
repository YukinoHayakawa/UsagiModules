#pragma once

namespace usagi
{
struct ServiceStateTransitionGraph
{
    using ServiceType = ServiceStateTransitionGraph;

    ServiceType & get_service() { return *this; }

    bool should_exit = false;
};
}
