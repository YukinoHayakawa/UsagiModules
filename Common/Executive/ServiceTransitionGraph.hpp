#pragma once

namespace usagi
{
struct ServiceTransitionGraph
{
    using ServiceType = ServiceTransitionGraph;

    ServiceType & get_service() { return *this; }

    bool should_exit = false;
};
}
