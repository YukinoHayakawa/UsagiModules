#pragma once

#include <Usagi/Library/Container/FixedCapacityString.hpp>

namespace usagi
{
struct ComponentNativeWindow
{
    FixedCapacityString<16> identifier;
    Vector2f dpi_scaling { 1, 1 };
};
}
