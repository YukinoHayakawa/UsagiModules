﻿#include "NativeWindowManagerWin32.hpp"

#include <algorithm>

#include <Usagi/Runtime/Memory/WeakSingleton.hpp>

#include "NativeWindowWin32.hpp"

namespace usagi
{
namespace
{
const wchar_t *gWin32WindowClassName = L"usagi::NativeWindowWin32";
}

namespace win32
{
struct Win32NativeWindowClass
{
    Win32NativeWindowClass()
    {
        USAGI_WIN32_CHECK_THROW(
            SetProcessDpiAwarenessContext,
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        );

        // get the process handle, all windows created using this class will have
        // their messages dispatched to our handler
        const auto process_instance_handle = GetModuleHandleW(nullptr);

        WNDCLASSEXW wcex { };
        wcex.cbSize = sizeof(WNDCLASSEXW);
        // CS_OWNDC is required to create OpenGL context
        wcex.style = CS_OWNDC;
        wcex.lpfnWndProc = &message_dispatcher;
        wcex.hInstance = process_instance_handle;
        // hInstance param must be null to use predefined cursors
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        // we print the background using graphics API like Vulkan/OpenGL/DirectX
        wcex.hbrBackground = nullptr;
        wcex.lpszClassName = gWin32WindowClassName;
        wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);

        USAGI_WIN32_CHECK_THROW(RegisterClassExW, &wcex);
    }

    ~Win32NativeWindowClass()
    {
        USAGI_WIN32_CHECK_ASSERT(
            UnregisterClassW,
            gWin32WindowClassName,
            GetModuleHandleW(nullptr)
        );
    }
};
}

/*
decltype(NativeWindowManagerWin32::mWindows)::iterator
NativeWindowManagerWin32::locate_window(std::string_view identifier)
{
    return std::ranges::find_if(
        mWindows,
        [&](auto &&r) { return r.identifier == identifier; }
    );
}
*/

NativeWindowManagerWin32::NativeWindowManagerWin32()
{
    using namespace win32;

    mWindowClass = WeakSingleton<Win32NativeWindowClass>::try_lock_construct();
}

NativeWindowManagerWin32::~NativeWindowManagerWin32()
{
}

std::shared_ptr<NativeWindow> NativeWindowManagerWin32::create_window(
    // std::string_view identifier,
    std::string_view title,
    const Vector2f &position,
    const Vector2f &size,
    float dpi_scaling,
    NativeWindowState state)
{
    // todo check unique id
    // WindowRecord record;
    // record.window =
    return std::make_unique<NativeWindowWin32>(
        title,
        position,
        size,
        dpi_scaling,
        state,
        gWin32WindowClassName
    );
    // record.identifier = identifier;
    // record.touched = true;
    // const auto p_wnd = record.window.get();
    // mWindows.emplace_back(std::move(record));
    // return p_wnd;
}

/*
void NativeWindowManagerWin32::destroy_window(std::string_view identifier)
{
    mWindows.erase(locate_window(identifier));
}

NativeWindow * NativeWindowManagerWin32::window(
    std::string_view identifier)
{
    const auto iter = locate_window(identifier);
    if(iter == mWindows.end())
        return nullptr;
    iter->touched = true;
    return iter->window.get();
}
*/

// void NativeWindowManagerWin32::destroy_unused_windows()
// {
//     const auto rng = std::ranges::remove_if(
//         mWindows,
//         [](auto &&wnd) {
//             if(wnd.touched == false || wnd.window->closed())
//             {
//                 wnd.window.reset();
//                 return true;
//             }
//             wnd.touched = false;
//             return false;
//         }
//     );
//     mWindows.erase(rng.begin(), rng.end());
// }

std::unique_ptr<NativeWindowManager>
NativeWindowManager::create_native_manager()
{
    return std::make_unique<NativeWindowManagerWin32>();
}
}
