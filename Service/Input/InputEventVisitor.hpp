#pragma once

#include "Keyboard/ArchetypeKeyboardEvent.hpp"
#include "Mouse/ArchetypeMouseEvent.hpp"

namespace usagi
{
class InputEventVisitor
{
public:
    virtual ~InputEventVisitor() = default;

    virtual void keyboard_event(
        const ArchetypeKeyboardEvent &archetype,
        bool pressed
    ) = 0;
    virtual void mouse_button_event(
        const ArchetypeMouseButtonEvent &archetype,
        bool pressed
    ) = 0;
    virtual void mouse_position_event(
        const ArchetypeMousePositionEvent &archetype
    ) = 0;
};
}
