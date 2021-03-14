#pragma once

#include <cstdint>

namespace usagi
{
enum class NativeWindowState : std::uint8_t
{
    CLOSED = 0,
    NORMAL = 1,
    HIDDEN = 2,
    MINIMIZED = 3,
    MAXIMIZED = 4,
};
}
