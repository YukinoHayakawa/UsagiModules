#include "RawInputSink.hpp"

#include <array>

namespace usagi
{
namespace detail::win32_input
{
LRESULT CALLBACK input_message_dispatcher(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
}

void RawInputSink::register_window_class()
{
    WNDCLASSEXW wcex { };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = &detail::win32_input::input_message_dispatcher;
    wcex.hInstance = process_instance_handle;
    wcex.lpszClassName = WINDOW_CLASS_NAME;

    if(!RegisterClassExW(&wcex))
        USAGI_WIN32_THROW("RegisterClassExW");
}

void RawInputSink::unregister_window_class()
{
    if(!UnregisterClassW(WINDOW_CLASS_NAME, process_instance_handle))
        USAGI_WIN32_THROW("UnregisterClassW");
}

void RawInputSink::create_input_sink_window()
{
    message_window = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"UsagiWin32RawInputSink",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, // create a message-only window
        nullptr,
        nullptr,
        nullptr
    );
    if(!message_window)
        USAGI_WIN32_THROW("CreateWindowExW");
}

void RawInputSink::destroy_input_sink_window()
{
    if(!DestroyWindow(message_window))
        USAGI_WIN32_THROW("DestroyWindow");
    message_window = nullptr;
}

void RawInputSink::register_raw_input_devices()
{
    std::array<RAWINPUTDEVICE, 3> devices;

    // For HID APIs, see:
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/
    // For usage page and usage codes, see:
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections-opened-by-windows-for-system-use
    // http://www.usb.org/developers/hidpage/Hut1_12v2.pdf

    // adds HID mice, RIDEV_NOLEGACY is not used because we need the system
    // to process non-client area.
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x02;
    // receives device add/remove messages (WM_INPUT_DEVICE_CHANGE)
    devices[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    // receives events from the window with keyboard focus
    devices[0].hwndTarget = message_window;

    // adds HID keyboards, RIDEV_NOLEGACY is not used to allow the system
    // process hotkeys like print screen. note that alt+f4 is not handled
    // if related key messages not passed to DefWindowProc(). looks like
    // RIDEV_NOLEGACY should only be used when having a single fullscreen
    // window.
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x06;
    // interestingly, RIDEV_NOHOTKEYS will prevent the explorer from using
    // the fancy window-choosing popup, and we still receive key events when
    // switching window, so it is not used here.
    devices[1].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    devices[1].hwndTarget = message_window;

    // adds gamepads
    devices[2].usUsagePage = 0x01;
    devices[2].usUsage = 0x05;
    devices[2].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    devices[2].hwndTarget = message_window;

    // note that this registration affects the entire application
    if(RegisterRawInputDevices(
        devices.data(), static_cast<UINT>(devices.size()),
        sizeof(RAWINPUTDEVICE)) == FALSE)
    {
        USAGI_WIN32_THROW("RegisterRawInputDevices");
    }
}
}
