#pragma once

#include "HeapResourceDescriptor.hpp"

namespace usagi
{
struct ResourceBuildOptions
{
    HeapResourceDescriptor requested_resource;
    /*
     * Another resource requesting current resource. If this is provided,
     * this function must be called from a builder, and a dependency edge
     * from current resource to the requester will be recorded.
     */
    HeapResourceDescriptor requesting_resource;
    HeapResourceDescriptor fallback_when_building;
    HeapResourceDescriptor fallback_if_failed;
    HeapResourceDescriptor fallback_if_evicted;
    bool rebuild_if_failed = false;
    bool rebuild_if_evicted = false;
};
}
