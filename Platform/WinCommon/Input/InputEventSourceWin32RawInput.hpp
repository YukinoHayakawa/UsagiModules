#pragma once

#include <memory>

#include <Usagi/Module/Service/Input/ServiceInputSource.hpp>

namespace usagi
{
// Not thread-safe.
class InputEventSourceWin32RawInput : public InputEventSource
{
    std::shared_ptr<struct RawInputSink> mSink;

public:
    InputEventSourceWin32RawInput();

    void collect_events() override;
    void process_events(InputEventInserter &input_event_sink) override;
};
}
