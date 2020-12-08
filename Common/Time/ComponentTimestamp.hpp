#pragma once

#include <cstdint>

namespace usagi
{
struct ComponentTimestamp
{
    // Seconds since the program/server start
    std::uint32_t seconds;
    // Nanosecond part
    std::uint32_t nanoseconds;
};
}
