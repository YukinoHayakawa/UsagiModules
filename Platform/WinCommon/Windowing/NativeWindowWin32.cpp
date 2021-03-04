#include "NativeWindowWin32.hpp"

namespace usagi
{
RECT NativeWindowWin32::window_rect() const
{
    RECT rect;
    rect.left = mPosition.x();
    rect.top = mPosition.y();
    rect.right = rect.left + mSize.x();
    rect.bottom = rect.top + mSize.y();
    AdjustWindowRectEx(&rect, WINDOW_STYLE, FALSE, WINDOW_STYLE_EX);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
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
    return style;
}

void NativeWindowWin32::update_style(const UINT style)
{
    SetWindowLongW(mHandle, GWL_STYLE, style ? style : build_style());
}

NativeWindowWin32::NativeWindowWin32(
    std::string_view title,
    const Vector2f &position,
    const Vector2f &size,
    const wchar_t *window_class)
{
    mPosition = position;
    mSize = size;

    // const auto window_title_wide = utf8To16(mTitle);
    const auto rect = window_rect();

    const auto dw_style = build_style();
    mHandle = CreateWindowExW(
        WINDOW_STYLE_EX,
        window_class,
        // window_title_wide.c_str(),
        L"test",
        dw_style,
        rect.left, rect.top,
        rect.right, rect.bottom,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );
    update_style(dw_style);

    if(!mHandle)
        USAGI_WIN32_THROW("CreateWindowEx");

    // show(true);
    ShowWindow(mHandle, SW_SHOWNORMAL);
}

NativeWindowWin32::~NativeWindowWin32()
{
    if(mHandle)
        NativeWindowWin32::destroy();
}

void NativeWindowWin32::destroy()
{
    DestroyWindow(mHandle);
    mHandle = nullptr;
}
}
