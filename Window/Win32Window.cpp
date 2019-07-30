﻿#include "Win32Window.hpp"

#include <Usagi/Utility/Unicode.hpp>
#include <Usagi/Core/Logging.hpp>
#include <Usagi/Utility/RAIIHelper.hpp>

#include "../Win32Helper.hpp"
#include "Win32WindowManager.hpp"

#include <versionhelpers.h>
#include <ShellScalingAPI.h>
#include "../Win32MacroFix.hpp"

RECT usagi::Win32Window::clientScreenRect() const
{
    // Get the window client area.
    RECT rc;
    GetClientRect(mHandle, &rc);

    POINT pt = { rc.left, rc.top };
    POINT pt2 = { rc.right, rc.bottom };
    ClientToScreen(mHandle, &pt);
    ClientToScreen(mHandle, &pt2);

    rc.left = pt.x;
    rc.top = pt.y;
    rc.right = pt2.x;
    rc.bottom = pt2.y;

    return rc;
}

UINT usagi::Win32Window::buildStyle() const
{
    UINT style = WS_VISIBLE;
    if(mFullscreen)
    {
        style |= WS_POPUP;
    }
    else
    {
        style |= WINDOW_STYLE;
        if(!mAllowResizing)
            style ^= WS_MAXIMIZEBOX | WS_SIZEBOX;
    }
    return style;
}

void usagi::Win32Window::updateWindowStyle()
{
    SetWindowLongW(mHandle, GWL_STYLE, buildStyle());
}

RECT usagi::Win32Window::getWindowRect() const
{
    RECT window_rect;
    window_rect.left = mPosition.x();
    window_rect.top = mPosition.y();
    window_rect.right = window_rect.left + mSize.x();
    window_rect.bottom = window_rect.top + mSize.y();
    AdjustWindowRectEx(&window_rect, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
    window_rect.right -= window_rect.left;
    window_rect.bottom -= window_rect.top;
    return window_rect;
}

void usagi::Win32Window::setResizingEnabled(const bool enabled)
{
    mAllowResizing = enabled;
    updateWindowStyle();
}

usagi::Win32Window::Win32Window(
    Win32WindowManager *manager,
    std::string title,
    Vector2i position,
    Vector2u32 size)
    : mTitle { std::move(title) }
    , mPosition { std::move(position) }
    , mSize { std::move(size) }
{
    LOG(info, "Creating window: {}", mTitle);

    const auto window_title_wide = utf8To16(mTitle);
    const auto window_rect = getWindowRect();

    mHandle = CreateWindowExW(
        WINDOW_STYLE_EX,
        Win32WindowManager::WINDOW_CLASS_NAME,
        window_title_wide.c_str(),
        buildStyle(),
        window_rect.left, window_rect.top,
        window_rect.right, window_rect.bottom,
        nullptr,
        nullptr,
        manager->processInstanceHandle(),
        nullptr
    );
    updateWindowStyle();

    if(!mHandle)
    {
        throw win32::Win32Exception("CreateWindowEx() failed");
    }

    Win32WindowManager::mWindows.insert({ mHandle, this });

    Win32Window::show(true);
}

usagi::Win32Window::~Win32Window()
{
    Win32WindowManager::mWindows.erase(mHandle);
}

HDC usagi::Win32Window::deviceContext() const
{
    return GetDC(mHandle);
}

void usagi::Win32Window::show(bool show)
{
    ShowWindow(mHandle, show ? SW_SHOWNORMAL : SW_HIDE);
}

bool usagi::Win32Window::focused() const
{
    return mFocused;
}

usagi::Vector2u32 usagi::Win32Window::size() const
{
    return mSize;
}

void usagi::Win32Window::updateWindowPosition(UINT flags) const
{
    const auto window_rect = getWindowRect();
    SetWindowPos(mHandle, nullptr,
        window_rect.left, window_rect.top, window_rect.right,
        window_rect.bottom,
        flags
    );
}

void usagi::Win32Window::sendResizeEvent()
{
    WindowSizeEvent e;
    e.window = this;
    e.size = mSize;
    forEachListener([&](auto h) {
        h->onWindowResizeBegin(e);
        h->onWindowResized(e);
        h->onWindowResizeEnd(e);
    });
}

void usagi::Win32Window::sendMoveEvent()
{
    WindowPositionEvent e;
    e.window = this;
    e.position = mPosition;
    forEachListener([&](auto h) {
        h->onWindowMoveBegin(e);
        h->onWindowMoved(e);
        h->onWindowMoveEnd(e);
    });
}

void usagi::Win32Window::enableBorderlessFullscreen()
{
    mFullscreen = true;
    updateWindowStyle();
    mPosition = { 0, 0 };
    mSize = Win32WindowManager::getCurrentDisplayResolution();
    SetWindowPos(mHandle, HWND_TOPMOST,
        mPosition.x(), mPosition.y(),
        mSize.x(), mSize.y(),
        SWP_FRAMECHANGED
    );
}

void usagi::Win32Window::disableBorderlessFullscreen()
{
    mFullscreen = false;
    updateWindowStyle();
    updateWindowPosition(SWP_FRAMECHANGED);
}

void usagi::Win32Window::setBorderlessFullscreen()
{
    // ref: https://stackoverflow.com/questions/34462445/fullscreen-vs-borderless-window

    enableBorderlessFullscreen();
    sendMoveEvent();
    sendResizeEvent();
}

std::string usagi::Win32Window::title() const
{
    // Don't use GetWindowText(), it may cause deadlock.
    return mTitle;
}

void usagi::Win32Window::setTitle(std::string title)
{
    mTitle = std::move(title);
    SetWindowTextW(mHandle, utf8To16(mTitle).c_str());
}

usagi::Vector2i usagi::Win32Window::position() const
{
    return mPosition;
}

void usagi::Win32Window::setSize(const Vector2u32 &size)
{
    mSize = size;
    if(mFullscreen)
        disableBorderlessFullscreen();
    else
        updateWindowPosition();
    sendResizeEvent();
}

void usagi::Win32Window::setPosition(const Vector2i &position)
{
    mPosition = position;
    if(mFullscreen)
        disableBorderlessFullscreen();
    else
        updateWindowPosition();
    sendMoveEvent();
}

void usagi::Win32Window::centerWindow()
{
    const auto display_size =
        Win32WindowManager::getCurrentDisplayResolution();
    const Vector2i centered_pos =
        (display_size.cast<int>() - size().cast<int>()) / 2;
    setPosition(centered_pos);
}

bool usagi::Win32Window::isOpen() const
{
    return !mClosed;
}

void usagi::Win32Window::close()
{
    DestroyWindow(mHandle);
}

usagi::Vector2f usagi::Win32Window::dpiScale() const
{
    unsigned int dpi_x = 96, dpi_y = 96;
    // http://www.virtualdub.org/blog/pivot/entry.php?id=384
    if(IsWindows8Point1OrGreater())
    {
        static HMODULE shcorelib = LoadLibraryExW(
            L"Shcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!shcorelib)
            throw win32::Win32Exception("Failed to load Shcore.dll.");
        static auto func_GetDpiForMonitor =
            reinterpret_cast<decltype(&GetDpiForMonitor)>(
            GetProcAddress(shcorelib, "GetDpiForMonitor"));
        if(!func_GetDpiForMonitor)
            throw win32::Win32Exception("Failed to find GetDpiForMonitor.");
        const auto monitor = MonitorFromWindow(
            mHandle, MONITOR_DEFAULTTONEAREST);
        func_GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    }
    else
    {
        if(HDC hdc = GetDC(NULL))
        {
            dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
            dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }
        else
        {
            LOG(warn, "Failed to get DPI scale.");
        }
    }
    return Vector2f { dpi_x, dpi_y } / 96.f;
}

// https://www-user.tu-chemnitz.de/~heha/petzold/ch12b.htm
std::string usagi::Win32Window::getClipboardText()
{
    if(!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return { };

    if(!OpenClipboard(mHandle))
    {
        LOG(error, "Could not open the clipboard to get text.");
        return { };
    }

    const auto hmem = GetClipboardData(CF_UNICODETEXT);
    void *pmem = nullptr;
    std::wstring str;
    {
        // auto unlock if wstring throws
        RAIIHelper raii_pmem {
            [&]() { pmem = GlobalLock(hmem); },
            [&]() { GlobalUnlock(hmem); }
        };
        str = {
            reinterpret_cast<wchar_t*>(pmem),
            GlobalSize(hmem) / sizeof(wchar_t) - 1
        };
    }

    CloseClipboard();

    return utf16To8(str);
}

void usagi::Win32Window::setClipboardText(const std::string &text)
{
    if(!OpenClipboard(mHandle))
    {
        LOG(error, "Could not open the clipboard");
        return;
    }

    const auto wstr = utf8To16(text);
    const auto hmem = GlobalAlloc(
        GHND | GMEM_SHARE, (wstr.size()  + 1) * sizeof(wchar_t));

    // copy text to allocated memory
    const auto pmem = GlobalLock(hmem);
    memcpy(pmem, wstr.data(), wstr.size() * sizeof(wchar_t));
    GlobalUnlock(hmem);

    EmptyClipboard();
    // the memory will be freed by next call of EmptyClipboard().
    SetClipboardData(CF_UNICODETEXT, hmem);

    CloseClipboard();
}

LRESULT usagi::Win32Window::handleWindowMessage(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    assert(hWnd == mHandle);

    // todo handle WM_DPICHANGED
    switch(message)
    {
        // window focus change
        case WM_ACTIVATE:
        {
            const auto active = LOWORD(wParam) != WA_INACTIVE;
            WindowFocusEvent e;
            e.window = this;
            e.focused = mFocused = active;
            forEachListener([&](auto h) {
                h->onWindowFocusChanged(e);
            });
            break;
        }
        // resizing & moving
        case WM_ENTERSIZEMOVE:
        {
            WindowSizeEvent e;
            e.window = this;
            e.size = size();
            e.sequence = mResizing = true;
            WindowPositionEvent e2;
            e2.window = this;
            e2.position = position();
            e2.sequence = mMoving = true;
            forEachListener([&](auto h) {
                h->onWindowResizeBegin(e);
                h->onWindowMoveBegin(e2);
            });
            break;
        }
        case WM_EXITSIZEMOVE:
        {
            WindowSizeEvent e;
            e.window = this;
            e.size = size();
            e.sequence = true;
            WindowPositionEvent e2;
            e2.window = this;
            e2.position = position();
            e2.sequence = true;
            forEachListener([&](auto h) {
                h->onWindowResizeEnd(e);
                h->onWindowMoveEnd(e2);
            });
            mResizing = mMoving = false;
            break;
        }
        case WM_SIZE:
        {
            WindowSizeEvent e;
            e.window = this;
            e.size = {
                LOWORD(lParam) < 0 ? 0 : LOWORD(lParam),
                HIWORD(lParam) < 0 ? 0 : HIWORD(lParam)
            };
            if(mSize == e.size) break;
            mSize = e.size;
            e.sequence = mResizing;
            forEachListener([&](auto h) {
                if(!e.sequence)
                {
                    h->onWindowResizeBegin(e);
                    h->onWindowResized(e);
                    h->onWindowResizeEnd(e);
                }
                else
                {
                    h->onWindowResized(e);
                }
            });
            break;
        }
        case WM_MOVE:
        {
            WindowPositionEvent e;
            e.window = this;
            // note that window position is signed
            auto x = static_cast<short>(LOWORD(lParam));
            auto y = static_cast<short>(HIWORD(lParam));
            e.position = { x, y };
            if(mPosition == e.position) break;
            mPosition = e.position;
            e.sequence = mMoving;
            forEachListener([&](auto h) {
                if(!e.sequence)
                {
                    h->onWindowMoveBegin(e);
                    h->onWindowMoved(e);
                    h->onWindowMoveEnd(e);
                }
                else
                {
                    h->onWindowMoved(e);
                }
            });
            break;
        }
        // WM_UNICHAR uses UTF-32 but is not sent for Unicode windows
        case WM_CHAR:
        {
            WindowCharEvent e;
            e.window = this;
            e.utf16 = static_cast<std::uint16_t>(wParam);
            if(isUtf16HighSurrogate(e.utf16))
                mHighSurrogate = e.utf16;
            else
                e.utf32 = isUtf16LowSurrogate(e.utf16)
                    ? utf16SurrogatesToUtf32(mHighSurrogate, e.utf16)
                    : e.utf16;
            codePointToUtf8(e.utf8, e.utf32);
            forEachListener([&](auto h) {
                h->onWindowCharInput(e);
            });
            break;
        }
        case WM_CLOSE:
        {
            // todo prompt the user
            DestroyWindow(mHandle);
            break;
        }
        case WM_DESTROY:
        {
            mClosed = true;
            break;
        }
        case WM_WINDOWPOSCHANGING:
        {
            break;
        }
        default:
        {
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}

HWND usagi::Win32Window::handle() const
{
    return mHandle;
}
