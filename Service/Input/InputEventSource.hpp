#pragma once

#include "InputEventInserter.hpp"

namespace usagi
{
class InputEventSource
{
public:
    virtual ~InputEventSource() = default;

    virtual void collect_events() = 0;
    virtual void process_events(InputEventInserter &input_event_sink) = 0;
};
}
