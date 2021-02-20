#pragma once

#include "InputEventInserter.hpp"

namespace usagi
{
class InputEventSource
{
public:
    virtual ~InputEventSource() = default;

    virtual bool pump_event(InputEventInserter &inserter) = 0;
};
}
