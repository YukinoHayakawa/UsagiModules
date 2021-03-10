#pragma once

#include <memory>

#include <Usagi/Module/Service/Input/ServiceInputSource.hpp>
#include <Usagi/Module/Platform/WinCommon/WindowMessageTarget.hpp>

namespace usagi
{
// Not thread-safe.
class InputEventSourceWin32RawInput : public InputEventSource
{
    std::shared_ptr<struct RawInputSink> mSink;

    bool raw_input__handle_keyboard(
        InputEventInserter &inserter,
        const tagRAWKEYBOARD &keyboard,
        const win32::MessageInfo &info);
    bool raw_input__handle_mouse(
        InputEventInserter &inserter,
        const tagRAWMOUSE &mouse,
        const win32::MessageInfo &info);

public:
    InputEventSourceWin32RawInput();

    void collect_events() override;
    void process_events(InputEventInserter &input_event_sink) override;
};
}
