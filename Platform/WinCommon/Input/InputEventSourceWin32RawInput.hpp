#pragma once

#include <memory>

#include <Usagi/Module/Service/Input/ServiceInputSource.hpp>

namespace usagi
{
namespace detail::win32_input
{
struct RawInputSink;
}

// Not thread-safe.
class InputEventSourceWin32RawInput : public InputEventSource
{
    std::shared_ptr<detail::win32_input::RawInputSink> mSink;

public:
    InputEventSourceWin32RawInput();

    void pump_events(InputEventVisitor &visitor) override;
};
}
