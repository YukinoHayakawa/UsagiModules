#include "WindowMessageTarget.hpp"

namespace usagi::win32
{
WindowMessageTarget * target_from_hwnd(const HWND window_handle)
{
    const auto proc = reinterpret_cast<void*>(
        GetClassLongPtrW(window_handle, GCLP_WNDPROC)
    );
    assert(proc == &message_dispatcher);
    const auto ptr = GetWindowLongPtrW(window_handle, GWLP_USERDATA);
    return reinterpret_cast<WindowMessageTarget*>(ptr);
}

void receive_messages()
{
    MSG msg;

    // PeekMessage only differs with GetMessage in that it doesn't block.
    // They internally both call xxxScanSysQueue which records the time of
    // the last fetched message in thread context.
    // Also note that scanning through the message queue could be expensive as
    // it traverses the message queue from the beginning every time. So avoid
    // using filters whenever possible.
    while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        DispatchMessageW(&msg);
    }
}

LRESULT message_dispatcher(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if(const auto target = target_from_hwnd(hWnd))
        target->message_handler(message, wParam, lParam);

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

std::chrono::milliseconds WindowMessageTarget::tick_to_clock(DWORD tick) const
{
    const auto milli = tick - mStartTime;
    return std::chrono::milliseconds(milli);
}
}
