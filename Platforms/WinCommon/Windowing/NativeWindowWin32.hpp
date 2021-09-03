#pragma once

#include <Usagi/Module/Platforms/WinCommon/WindowMessageTarget.hpp>
#include <Usagi/Module/Service/Windowing/NativeWindow.hpp>

#include "NativeWindowManagerWin32.hpp"
#include "../Win32.hpp"

namespace usagi
{
class NativeWindowWin32
    : public NativeWindow
    , public win32::WindowMessageTarget
{
    bool mFullscreen = false;
    bool mAllowResizing = true;

    static constexpr DWORD WINDOW_STYLE = WS_OVERLAPPEDWINDOW;
    static constexpr DWORD WINDOW_STYLE_EX = WS_EX_ACCEPTFILES;

    RECT window_rect() const;
    UINT build_style() const;
    void update_style(UINT style);
    void verify_client_area_size();
    void update_position();
    void sync_window_state();

    void update_dpi_scaling(UINT dpi)
    {
        mDpiScaling = dpi;
        mDpiScaling /= USER_DEFAULT_SCREEN_DPI;
        mSurfaceSize = mLogicalSize * mDpiScaling;
    }

    friend class NativeWindowManagerWin32;

public:
    NativeWindowWin32(
        std::string_view title,
        const Vector2f &position,
        const Vector2f &size,
        float dpi_scaling,
        NativeWindowState state,
        const wchar_t *window_class
    );

    void destroy() override;

    HWND handle() const { return mWindowHandle; }

    LRESULT message_handler(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) override;
};
}
