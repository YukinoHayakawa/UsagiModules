#pragma once

#include <Usagi/Module/Common/Math/Matrix.hpp>
#include <Usagi/Module/Service/Input/ArchetypeInputEvent.hpp>

#include "MouseButtonCode.hpp"

namespace usagi
{
struct ComponentMouseButtonCode
{
    MouseButtonCode code;
};

using ArchetypeMouseButtonEvent = ArchetypeInputEvent::Derived<
    ComponentMouseButtonCode
>;
static_assert(std::is_same_v<
    ArchetypeMouseButtonEvent,
    Archetype<
        ComponentTimestamp,
        ComponentMouseButtonCode
    >
>);

struct ComponentMousePosition
{
    Vector2f cursor_pos;
    Vector2f movement;
};

using ArchetypeMousePositionEvent = ArchetypeInputEvent::Derived<
    ComponentMousePosition
>;
static_assert(std::is_same_v<
    ArchetypeMousePositionEvent,
    Archetype<
        ComponentTimestamp,
        ComponentMousePosition
    >
>);
}
