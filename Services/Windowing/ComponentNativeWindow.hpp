#pragma once

#include <Usagi/Library/Container/FixedCapacityString.hpp>

#include "NativeWindowState.hpp"

namespace usagi
{
struct ComponentNativeWindow
{
    FixedCapacityString<16> identifier;
    float dpi_scaling = 1;

    enum OnCloseAction : std::uint8_t
    {
        // IGNORE          = 0,
        // HIDE            = 1 << 0,
        DESTROY_ENTITY  = 1 << 1,
        NOTIFY_EXIT     = 1 << 2,
    } on_close = DESTROY_ENTITY;

    NativeWindowState state = NativeWindowState::NORMAL;
};
USAGI_DECL_TAG_COMPONENT(TagNativeWindowDestroy);
}

