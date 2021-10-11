#pragma once

#include <chrono>

#include <Usagi/Library/Memory/Noncopyable.hpp>

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

class WindowMessageTarget : Noncopyable
{
protected:
    HWND mWindowHandle = nullptr;

    // the number of milliseconds that have elapsed since the system was started
    DWORD mStartTime = GetTickCount();
    ULONGLONG mStartTime64 = GetTickCount64();
    std::chrono::high_resolution_clock::time_point mStartTimeWall =
        std::chrono::high_resolution_clock::now();

public:
    WindowMessageTarget() = default;
    WindowMessageTarget(WindowMessageTarget &&other) noexcept;
    virtual ~WindowMessageTarget();

    WindowMessageTarget & operator=(WindowMessageTarget &&other) noexcept;

    virtual LRESULT message_handler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) = 0;

    // todo: handle overflow
    // https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount
    std::chrono::milliseconds tick_to_clock(DWORD tick) const;
};
}
