#pragma once

#include "Win32.hpp"

namespace usagi::win32
{
// constexpr LONG_PTR WINDOW_MAGIC_ID = 0xbb2b5c5488e592cb;

void receive_messages();
LRESULT message_dispatcher(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

struct MessageInfo
{
    POINTS cursor;
    // https://devblogs.microsoft.com/oldnewthing/20140122-00/?p=2013
    ULONG time;
};
static_assert(sizeof(MessageInfo) == sizeof(WPARAM));

class WindowMessageTarget
{
protected:
    HWND mWindowHandle = nullptr;

public:
    virtual ~WindowMessageTarget() = default;

    virtual LRESULT message_handler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) = 0;
};
}
