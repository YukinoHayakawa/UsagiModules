#include "Input.hpp"
#include "InputEventSourceWin32RawInput.hpp"

#include <vector>

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>
#include <Usagi/Runtime/WeakSingleton.hpp>

#include "RawInputSink.hpp"

namespace usagi
{
InputEventSourceWin32RawInput::InputEventSourceWin32RawInput()
{
    mSink = WeakSingleton<RawInputSink>::try_lock_construct();
}

void InputEventSourceWin32RawInput::collect_events()
{
    win32::receive_messages();
}

void InputEventSourceWin32RawInput::process_events(
    InputEventInserter &input_event_sink)
{
    using namespace win32;

    auto queue = mSink->message_queue;
    auto head = reinterpret_cast<RAWINPUT*>(queue.data());
    const auto end = reinterpret_cast<std::size_t>(queue.data()) + queue.size();

    while(head && reinterpret_cast<std::size_t>(head) < end)
    {
        const auto &info = *(MessageInfo*)&head->header.wParam;

        switch(head->header.dwType)
        {
            case RIM_TYPEKEYBOARD:
                raw_input__handle_keyboard(
                    input_event_sink,
                    head->data.keyboard,
                    info
                );
                break;

            case RIM_TYPEMOUSE:
                raw_input__handle_mouse(
                    input_event_sink,
                    head->data.mouse,
                    info
                );
                break;

            case RIM_TYPEHID:
            default:
                break;
        }
        head = reinterpret_cast<RAWINPUT*>(
            reinterpret_cast<std::size_t>(head) + head->header.dwSize
        );
    }

    mSink->message_queue.clear();
}
}
