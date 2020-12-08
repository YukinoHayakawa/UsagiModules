#pragma once

#include <Usagi/Module/Service/Input/ArchetypeInputEvent.hpp>

#include "KeyCode.hpp"

namespace usagi
{
USAGI_DECL_TAG_COMPONENT(TagKeyRepeated);

struct ComponentKeyCode
{
    KeyCode code;
};

using ArchetypeKeyboardEvent = ArchetypeInputEvent::Derived<
    ComponentKeyCode
>;
static_assert(std::is_same_v<
    ArchetypeKeyboardEvent,
    Archetype<
        ComponentTimestamp,
        ComponentKeyCode
    >
>);
}
