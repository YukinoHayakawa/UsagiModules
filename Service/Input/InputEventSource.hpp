#pragma once

namespace usagi
{
class InputEventVisitor;

class InputEventSource
{
public:
    virtual ~InputEventSource() = default;

    virtual void pump_events(InputEventVisitor &visitor) = 0;
};
}
