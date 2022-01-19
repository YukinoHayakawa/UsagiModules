#pragma once

#include "HeapResourceDescriptor.hpp"

namespace usagi
{
struct ResourceBuildOptions
{
    HeapResourceDescriptor requested_resource;
    HeapResourceDescriptor requesting_resource;
    HeapResourceDescriptor fallback_when_building;
    HeapResourceDescriptor fallback_if_failed;
    HeapResourceDescriptor fallback_if_evicted;
    bool rebuild_if_failed = false;
    bool rebuild_if_evicted = false;
};
}
