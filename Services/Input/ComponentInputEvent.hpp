#pragma once

#include <Usagi/Modules/Common/Math/Matrix.hpp>

#include "InputAxis.hpp"

namespace usagi
{
struct ComponentInputEvent
{
    // Key pressed and axial movements are modeled using two dimensional
    // positions. The coordinate system is supposed to be left-handed:
    //
    //  +---- x
    //  |
    //  |
    //  y

    // For keyboard:
    //   Key Press:           x=0, y=1
    //   Key Release:         x=0, y=0
    // For mouse:
    //   Button Press:        x=0, y=1
    //   Button Release:      x=0, y=0
    //   Wheel:               x=0, y=0 [no absolute position]
    //   Movement:            [coordinates on the desktop]
    Vector2f absolute;

    // For keyboard:
    //   Key Press:           x=0, y=1
    //   Key Release:         x=0, y=-1
    // For mouse:
    //   Button Press:        x=0, y=1
    //   Button Release:      x=0, y=-1
    //   Wheel (Vertical):    x=0, y=[event value]
    //   Wheel (Horizontal):  x=[event value], y=0
    //   Movement:            [event values]
    Vector2f relative;

    InputAxis axis;

    bool pressed() const
    {
        return absolute.y() != 0;
    }

    bool released() const
    {
        return absolute.y() == 0;
    }
};
}
