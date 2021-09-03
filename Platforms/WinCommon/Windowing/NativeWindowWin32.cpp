#include "NativeWindowWin32.hpp"

// #include <fmt/printf.h>

namespace usagi
{
RECT NativeWindowWin32::window_rect() const
{
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    // Treated as width and height for the convenience of using SetWindowPos.
    // AdjustWindowRect doesn't really care about the values.
    rect.right = mSurfaceSize.x();
    rect.bottom = mSurfaceSize.y();
    AdjustWindowRectExForDpi(
        &rect,
        WINDOW_STYLE,
        FALSE,
        WINDOW_STYLE_EX,
        mDpiScaling * USER_DEFAULT_SCREEN_DPI
    );
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left += mPosition.x();
    rect.top += mPosition.y();

    return rect;
}

UINT NativeWindowWin32::build_style() const
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
    if(mState == NativeWindowState::MINIMIZED) style |= WS_MINIMIZE;
    else if(mState == NativeWindowState::MAXIMIZED) style |= WS_MAXIMIZE;
    return style;
}

void NativeWindowWin32::update_style(const UINT style)
{
    SetWindowLongW(mWindowHandle, GWL_STYLE, style ? style : build_style());
}

void NativeWindowWin32::verify_client_area_size()
{
    RECT c_rect;
    USAGI_WIN32_CHECK_THROW(
        GetClientRect,
        mWindowHandle,
        &c_rect
    );
    assert(c_rect.right == mSurfaceSize.x());
    assert(c_rect.bottom == mSurfaceSize.y());
}

void NativeWindowWin32::update_position()
{
    const auto rect = window_rect();
    USAGI_WIN32_CHECK_THROW(
        SetWindowPos,
        mWindowHandle,
        nullptr,
        rect.left, rect.top,
        rect.right, rect.bottom,
        SWP_NOZORDER | SWP_NOACTIVATE
    );
    // verify_client_area_size();
}

void NativeWindowWin32::sync_window_state()
{
    int flag = 0;
    switch(mState)
    {
        case NativeWindowState::NORMAL: flag = SW_SHOWNORMAL; break;
        case NativeWindowState::HIDDEN: flag = SW_HIDE; break;
        case NativeWindowState::MINIMIZED: flag = SW_SHOWMINIMIZED; break;
        case NativeWindowState::MAXIMIZED: flag = SW_SHOWMAXIMIZED; break;
        default: USAGI_UNREACHABLE();
    }
    USAGI_WIN32_CHECK_THROW(ShowWindow, mWindowHandle, flag);
}

NativeWindowWin32::NativeWindowWin32(
    std::string_view title,
    const Vector2f &position,
    const Vector2f &size,
    float dpi_scaling,
    NativeWindowState state,
    const wchar_t *window_class)
    : NativeWindow(position, size, dpi_scaling, state)
{
    update_dpi_scaling(USER_DEFAULT_SCREEN_DPI * dpi_scaling);

    // todo
    // const auto window_title_wide = utf8To16(mTitle);
    const auto rect = window_rect();
    const auto dw_style = build_style();

    USAGI_WIN32_CHECK_ASSIGN_THROW(
        mWindowHandle,
        CreateWindowExW,
        WINDOW_STYLE_EX,
        window_class,
        // window_title_wide.c_str(),
        L"test",
        dw_style,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    SetWindowLongPtrW(
        mWindowHandle,
        GWLP_USERDATA,
        reinterpret_cast<ULONG_PTR>(static_cast<WindowMessageTarget*>(this))
    );

    // todo: handle changes in monitor position/DPI between runs of the program.
    if(mState != NativeWindowState::MINIMIZED)
    {
        // Update window DPI scaling
        UINT dpi;
        USAGI_WIN32_CHECK_ASSIGN_THROW(dpi, GetDpiForWindow, mWindowHandle);
        update_dpi_scaling(dpi);
        update_position();
    }

    USAGI_WIN32_CHECK_THROW(ShowWindow, mWindowHandle, SW_SHOW);
}

void NativeWindowWin32::destroy()
{
    USAGI_WIN32_CHECK_ASSERT(DestroyWindow, mWindowHandle);
    mWindowHandle = nullptr;
}

LRESULT NativeWindowWin32::message_handler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
        case WM_CLOSE:
        {
            mState = NativeWindowState::CLOSED;
            return 0;
        }

        // https://docs.microsoft.com/en-us/windows/win32/hidpi/wm-getdpiscaledsize
        // Posted just before WM_DPICHANGED.
        // bug: the value returned from this message is NOT passed to WM_DPICHANGED, at least in Windows 10 2004. Probably a Windows bug.
        case WM_GETDPISCALEDSIZE:
        {
            // fmt::print("WM_GETDPISCALEDSIZE\n");

            update_dpi_scaling(wParam);

            // Calculate the new decorated size and provide it to the OS.
            SIZE * const new_size = reinterpret_cast<PSIZE>(lParam);
            const auto rect = window_rect();
            new_size->cx = rect.right;
            new_size->cy = rect.bottom;

            return TRUE;
        }

        // https://docs.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged
        // Note: Set the application to PROCESS_PER_MONITOR_DPI_AWARE
        // to receive this message.
        case WM_DPICHANGED:
        {
            // fmt::print("WM_DPICHANGED\n");

            update_dpi_scaling(HIWORD(wParam));

            // The RECT calculated by Windows is inaccurate: moving a window
            // back and forth between two monitors with different scaling could
            // change the size of the window. Therefore, we calculate the size
            // by ourselves. Unless the window is being maximized (by snapping,
            // for example), in which case a sequence of WM_MOVE, WM_SIZE,
            // WM_DPICHANGED is passed, and two subsequent WM_MOVE and
            // WM_SIZE will be caused by the DPI change. The first move & size
            // messages will update our rect of the window. If we rescale the
            // window again during the DPI change, the window will have
            // incorrect size.
            RECT* const prc_new_window = (RECT*)lParam;
            if(mState == NativeWindowState::MAXIMIZED)
            {
                USAGI_WIN32_CHECK_THROW(
                    SetWindowPos,
                    mWindowHandle,
                    nullptr,
                    prc_new_window->left,
                    prc_new_window->top,
                    prc_new_window->right - prc_new_window->left,
                    prc_new_window->bottom - prc_new_window->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            else
            {
                const auto rect = window_rect();
                USAGI_WIN32_CHECK_THROW(
                    SetWindowPos,
                    mWindowHandle,
                    nullptr,
                    prc_new_window->left,
                    prc_new_window->top,
                    rect.right,
                    rect.bottom,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            // verify_client_area_size();

            return 0;
        }

        // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-move
        case WM_MOVE:
        {
            // fmt::print("WM_MOVE\n");

            const auto x = (int)(short)LOWORD(lParam);
            const auto y = (int)(short)HIWORD(lParam);

            // The window is minimized.
            // https://stackoverflow.com/questions/31755601/does-minimizing-a-window-on-a-modern-version-of-windows-still-move-it-to-coordin
            if(!(x == -32000 && y == -32000))
                mPosition = { x, y };

            return 0;
        }

        case WM_SIZE:
        {
            // fmt::print("WM_SIZE\n");

            switch(wParam)
            {
                case SIZE_MAXIMIZED:
                    mState = NativeWindowState::MAXIMIZED;
                    break;
                // Don't update window size if it is minimized, since the size
                // would be 0.
                case SIZE_MINIMIZED:
                    mState = NativeWindowState::MINIMIZED;
                    return 0;
                case SIZE_RESTORED:
                    mState = NativeWindowState::NORMAL;
                    break;
                default: goto defproc;
            }

            mSurfaceSize = { LOWORD(lParam), HIWORD(lParam) };
            mLogicalSize = mSurfaceSize / mDpiScaling;
            mLogicalSize = { ceil(mLogicalSize.x()), ceil(mLogicalSize.y()) };

            return 0;
        }

        default: break;
    }
defproc:
    return DefWindowProcW(mWindowHandle, message, wParam, lParam);
}
}
