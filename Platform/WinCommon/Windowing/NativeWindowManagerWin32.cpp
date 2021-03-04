#include "NativeWindowManagerWin32.hpp"

#include <algorithm>

#include "NativeWindowWin32.hpp"

namespace usagi
{
namespace detail::win32
{
NativeWindowManagerWin32 *gCurrentWindowManager = nullptr;

LRESULT process_window_message(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    const auto wnd = gCurrentWindowManager->window_from_handle(hWnd);
    if(wnd == nullptr)
        return DefWindowProcW(hWnd, message, wParam, lParam);

    // todo handle WM_DPICHANGED
    switch(message)
    {
        // // window focus change
        // case WM_ACTIVATE:
        // {
        //     const auto active = LOWORD(wParam) != WA_INACTIVE;
        //     WindowFocusEvent e;
        //     e.window = this;
        //     e.focused = mFocused = active;
        //     forEachListener([&](auto h) {
        //         h->onWindowFocusChanged(e);
        //     });
        //     break;
        // }
        // // resizing & moving
        // case WM_ENTERSIZEMOVE:
        // {
        //     WindowSizeEvent e;
        //     e.window = this;
        //     e.size = size();
        //     e.sequence = mResizing = true;
        //     WindowPositionEvent e2;
        //     e2.window = this;
        //     e2.position = position();
        //     e2.sequence = mMoving = true;
        //     forEachListener([&](auto h) {
        //         h->onWindowResizeBegin(e);
        //         h->onWindowMoveBegin(e2);
        //     });
        //     break;
        // }
        // case WM_EXITSIZEMOVE:
        // {
        //     WindowSizeEvent e;
        //     e.window = this;
        //     e.size = size();
        //     e.sequence = true;
        //     WindowPositionEvent e2;
        //     e2.window = this;
        //     e2.position = position();
        //     e2.sequence = true;
        //     forEachListener([&](auto h) {
        //         h->onWindowResizeEnd(e);
        //         h->onWindowMoveEnd(e2);
        //     });
        //     mResizing = mMoving = false;
        //     break;
        // }
        // case WM_SIZE:
        // {
        //     WindowSizeEvent e;
        //     e.window = this;
        //     e.size = {
        //         LOWORD(lParam) < 0 ? 0 : LOWORD(lParam),
        //         HIWORD(lParam) < 0 ? 0 : HIWORD(lParam)
        //     };
        //     e.sequence = mResizing;
        //     switch(wParam)
        //     {
        //         // minimize & restore events might be used to pause the
        //         // rendering
        //         case SIZE_MINIMIZED:
        //         {
        //             forEachListener([&](auto h) {
        //                 h->onWindowMinimized(e);
        //             });
        //             break;
        //         }
        //
        //         case SIZE_RESTORED:
        //         {
        //             forEachListener([&](auto h) {
        //                 h->onWindowRestored(e);
        //             });
        //             // in case the size is also changed
        //             [[fallthrough]];
        //         }
        //
        //         default:
        //         {
        //             if(mSize == e.size) break;
        //             mSize = e.size;
        //             forEachListener([&](auto h) {
        //                 if(!e.sequence)
        //                 {
        //                     h->onWindowResizeBegin(e);
        //                     h->onWindowResized(e);
        //                     h->onWindowResizeEnd(e);
        //                 }
        //                 else
        //                 {
        //                     h->onWindowResized(e);
        //                 }
        //             });
        //         }
        //     }
        //     break;
        // }
        // case WM_MOVE:
        // {
        //     WindowPositionEvent e;
        //     e.window = this;
        //     // note that window position is signed
        //     auto x = static_cast<short>(LOWORD(lParam));
        //     auto y = static_cast<short>(HIWORD(lParam));
        //     e.position = { x, y };
        //     if(mPosition == e.position) break;
        //     mPosition = e.position;
        //     e.sequence = mMoving;
        //     forEachListener([&](auto h) {
        //         if(!e.sequence)
        //         {
        //             h->onWindowMoveBegin(e);
        //             h->onWindowMoved(e);
        //             h->onWindowMoveEnd(e);
        //         }
        //         else
        //         {
        //             h->onWindowMoved(e);
        //         }
        //     });
        //     break;
        // }
        // // WM_UNICHAR uses UTF-32 but is not sent for Unicode windows
        // case WM_CHAR:
        // {
        //     WindowCharEvent e;
        //     e.window = this;
        //     e.utf16 = static_cast<std::uint16_t>(wParam);
        //     if(isUtf16HighSurrogate(e.utf16))
        //         mHighSurrogate = e.utf16;
        //     else
        //         e.utf32 = isUtf16LowSurrogate(e.utf16)
        //             ? utf16SurrogatesToUtf32(mHighSurrogate, e.utf16)
        //             : e.utf16;
        //     codePointToUtf8(e.utf8, e.utf32);
        //     forEachListener([&](auto h) {
        //         h->onWindowCharInput(e);
        //     });
        //     break;
        // }
        // case WM_CLOSE:
        // {
        //     // todo prompt the user
        //     DestroyWindow(mHandle);
        //     break;
        // }
        case WM_DESTROY:
        {
            NativeWindowManagerWin32::mark_closed(wnd);
            break;
        }
        // prevent window size from being restricted by desktop size
        // https://stackoverflow.com/questions/445893/create-window-larger-than-desktop-display-resolution
        case WM_WINDOWPOSCHANGING:
        {
            // break;
        }
        default:
        {
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}

const wchar_t *gWin32WindowClassName = L"UsagiNativeWindowWin32";

struct Win32NativeWindowClass
{
    Win32NativeWindowClass()
    {
        // get the process handle, all windows created using this class will have
        // their messages dispatched to our handler
        const auto process_instance_handle = GetModuleHandleW(nullptr);

        WNDCLASSEXW wcex { };
        wcex.cbSize = sizeof(WNDCLASSEXW);
        // CS_OWNDC is required to create OpenGL context
        wcex.style = CS_OWNDC;
        wcex.lpfnWndProc = &process_window_message;
        wcex.hInstance = process_instance_handle;
        // hInstance param must be null to use predefined cursors
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        // we print the background using graphics API like Vulkan/OpenGL/DirectX
        wcex.hbrBackground = nullptr;
        wcex.lpszClassName = gWin32WindowClassName;
        wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);

        if(!RegisterClassExW(&wcex))
            USAGI_WIN32_THROW("RegisterClassEx");
    }

    ~Win32NativeWindowClass()
    {
        UnregisterClassW(gWin32WindowClassName, GetModuleHandleW(nullptr));
    }
};

std::weak_ptr<Win32NativeWindowClass> gWin32NativeWindowClass;
}

void NativeWindowManagerWin32::mark_closed(NativeWindowWin32 *window)
{
    window->mClosed = true;
}

NativeWindowManagerWin32::NativeWindowManagerWin32()
{
    using namespace detail::win32;

    mWindowClass = gWin32NativeWindowClass.lock();
    if(!mWindowClass)
    {
        mWindowClass = std::make_shared<Win32NativeWindowClass>();
        gWin32NativeWindowClass = mWindowClass;
    }
}

NativeWindowManagerWin32::~NativeWindowManagerWin32()
{
}

NativeWindow * NativeWindowManagerWin32::create_window(
    std::string_view identifier,
    std::string_view title,
    const Vector2f &position,
    const Vector2f &size)
{
    WindowRecord record;
    record.window = std::make_unique<NativeWindowWin32>(
        title, position, size, detail::win32::gWin32WindowClassName
    );
    record.identifier = identifier;
    record.touched = true;
    const auto p_wnd = record.window.get();
    const auto h_wnd = record.window->handle();
    mWindows.emplace(h_wnd, std::move(record));
    return p_wnd;
}

NativeWindow * NativeWindowManagerWin32::window(
    std::string_view identifier)
{
    const auto iter = std::ranges::find_if(mWindows, [&](auto &&r) {
        return r.second.identifier == identifier;
    });
    if(iter == mWindows.end()) return nullptr;
    iter->second.touched = true;
    return iter->second.window.get();
}

void NativeWindowManagerWin32::destroy_unused_windows()
{
    for(auto i = mWindows.begin(); i != mWindows.end();)
    {
        auto &snd = i->second;
        if(snd.touched == false || snd.window->closed())
        {
            snd.window.reset();
            i = mWindows.erase(i);
        }
        else
        {
            snd.touched = false;
            ++i;
        }
    }
}

NativeWindowWin32 * NativeWindowManagerWin32::window_from_handle(void *hwnd)
{
    const auto iter = std::ranges::find_if(mWindows, [&](auto &&r) {
        return r.second.window->handle() == hwnd;
    });
    if(iter == mWindows.end()) return nullptr;
    return iter->second.window.get();
}

// void NativeWindowManagerWin32::process_events()
// {
//     // bug outdated doc
//     // Win32WindowManager and Win32InputManager both has a message loop that
//     // processes messages for any windows in the current threadaaaa to prevent
//     // leaving some messages unprocessed in the queue, which causes Windows
//     // to believe our windows are unresponsive. therefore the order of
//     // calling processEvents() on these two managers is insignificant on
//     // Windows.
//
//     using namespace detail::win32;
//
//     assert(gCurrentWindowManager == nullptr);
//     gCurrentWindowManager = this;
//
//     MSG msg;
//     for(auto &&wnd : mWindows)
//     {
//         const auto hwnd = static_cast<HWND>(wnd.first);
//         while(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE))
//         {
//             TranslateMessage(&msg);
//             DispatchMessageW(&msg);
//         }
//     }
//
//     gCurrentWindowManager = nullptr;
// }
}
