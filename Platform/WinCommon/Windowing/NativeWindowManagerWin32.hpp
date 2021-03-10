#pragma once

#include <vector>

#include <Usagi/Library/Container/FixedCapacityString.hpp>
#include <Usagi/Module/Service/Windowing/NativeWindowManager.hpp>

namespace usagi
{
namespace win32
{
struct Win32NativeWindowClass;
}

class NativeWindowManagerWin32 : public NativeWindowManager
{
    struct WindowRecord
    {
        std::unique_ptr<class NativeWindowWin32> window;
        FixedCapacityString<16> identifier;
        bool touched = true;
    };
    // HWND -> Record
    std::vector<WindowRecord> mWindows;
    std::shared_ptr<win32::Win32NativeWindowClass> mWindowClass;

public:
    NativeWindowManagerWin32();
    ~NativeWindowManagerWin32();

    NativeWindow * create_window(
        std::string_view identifier,
        std::string_view title,
        const Vector2f &position,
        const Vector2f &size) override;
    NativeWindow * window(std::string_view identifier) override;
    void destroy_unused_windows() override;

    static void mark_closed(NativeWindowWin32 *window);
};
}
