#pragma once

#include <Usagi/Library/Container/FixedCapacityString.hpp>

namespace usagi
{
struct ComponentNativeWindow
{
    FixedCapacityString<16> identifier;
    Vector2f dpi_scaling { 1, 1 };

    enum class OnCloseAction : std::uint16_t
    {
        IGNORE          = 0,
        HIDE            = 1 << 0,
        DESTROY_ENTITY  = 1 << 1,
        NOTIFY_EXIT     = 1 << 2,
    } on_close;
};
}
