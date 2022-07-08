#pragma once

#include <vector>

#include <Usagi/Library/Container/FixedCapacityString.hpp>
#include <Usagi/Modules/IO/Windowing/NativeWindowManager.hpp>

namespace usagi
{
namespace win32
{
struct Win32NativeWindowClass;
}

class NativeWindowManagerWin32 : public NativeWindowManager
{
    // must be destructed after destroying all the window
    std::shared_ptr<win32::Win32NativeWindowClass> mWindowClass;

    /*
    struct WindowRecord
    {
        std::unique_ptr<class NativeWindowWin32> window;
        FixedCapacityString<16> identifier;
        bool touched = true; // todo remove. use heap res refcnt.
    };
    // HWND -> Record
    std::vector<WindowRecord> mWindows;
    */

    // decltype(mWindows)::iterator locate_window(std::string_view identifier);

public:
    // Create the class using NativeWindowManager::create_native_manager.
    NativeWindowManagerWin32();
    ~NativeWindowManagerWin32() override;

    std::shared_ptr<NativeWindow> create_window(
        // std::string_view identifier,
        std::string_view title,
        const Vector2f &position,
        const Vector2f &size,
        float dpi_scaling,
        NativeWindowState state) override;
    // void destroy_window(std::string_view identifier) override;

    // NativeWindow * window(std::string_view identifier) override;

    // void destroy_unused_windows() override;

    using NativeWindowManager::create_native_manager;
};
}
