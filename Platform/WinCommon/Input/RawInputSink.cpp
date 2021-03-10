#include "RawInputSink.hpp"

#include <array>

namespace usagi
{
namespace
{
constexpr auto WINDOW_CLASS_NAME = L"usagi::RawInputSink";
}

namespace win32
{
RAWINPUT* get_raw_input_data(LPARAM lParam)
{
    // RAWHID may have data larger than the RAWINPUT structure.
    thread_local static std::vector<BYTE> buffer;
    UINT buf_size = static_cast<UINT>(buffer.size());

    // Fetch raw input data.
    // Note that WM_INPUT messages cannot be stored. When processing a WM_INPUT
    // message in PeekMessage/GetMessage, any unretrieved previous HIDDATA
    // will be freed by FreeHidData (see xxxRealInternalGetMessage).
    int ret = GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        buffer.data(), // nullptr during the first call
        &buf_size,
        sizeof(RAWINPUTHEADER)
    );

/*
  ntstubs.c:NtUserGetRawInputData:

  Situation                                LastError                  Return
  ---------------------------------------- -------------------------- ---------
  cbSizeHeader != sizeof(RAWINPUTHEADER)   ERROR_INVALID_PARAMETER    -1
  invalid handle                           0                          -1
  Bad dwType for pHidData                  0                          -1
  invalid uiCommand                        ERROR_INVALID_PARAMETER    -1
  !pData && can't write pcbSize            0                          -1
  !pData && buffer size wrote to pcbSize   -                          0
  pData && cbBufferSize >= cbOutSize       -                          cbOutSize
  pData && cbBufferSize < cbOutSize        ERROR_INSUFFICIENT_BUFFER  -1
*/

    if((ret == -1 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) || ret == 0)
    {
        // resize the buffer and try again
        buffer.resize(buf_size);
        ret = GetRawInputData(
            reinterpret_cast<HRAWINPUT>(lParam),
            RID_INPUT,
            buffer.data(),
            &buf_size,
            sizeof(RAWINPUTHEADER)
        );
        if(ret != buf_size) USAGI_WIN32_THROW("GetRawInputData");
    }
    else if(ret == -1) USAGI_WIN32_THROW("GetRawInputData");

    return reinterpret_cast<RAWINPUT*>(buffer.data());
}
}

void RawInputSink::register_window_class()
{
    WNDCLASSEXW wcex {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = &win32::message_dispatcher,
        .hInstance = process_instance_handle,
        .lpszClassName = WINDOW_CLASS_NAME
    };

    USAGI_WIN32_CHECK_THROW(RegisterClassExW, &wcex);
}

void RawInputSink::unregister_window_class()
{
    USAGI_WIN32_CHECK_THROW(
        UnregisterClassW,
        WINDOW_CLASS_NAME, process_instance_handle
    );
}

void RawInputSink::create_input_sink_window()
{
    USAGI_WIN32_CHECK_ASSIGN_THROW(
        mWindowHandle,
        CreateWindowExW,
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

    SetWindowLongPtrW(mWindowHandle, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(this));
}

void RawInputSink::destroy_input_sink_window()
{
    // do not throw exception in destructor
    USAGI_WIN32_CHECK_ASSERT(DestroyWindow, mWindowHandle);
    mWindowHandle = nullptr;
}

LRESULT RawInputSink::message_handler(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
        // simply store the messages for later processing
        case WM_INPUT:
        {
            const auto current_size = message_queue.size();
            const auto input = win32::get_raw_input_data(lParam);
            message_queue.resize(current_size + input->header.dwSize);
            std::memcpy(
                &message_queue[current_size],
                input,
                input->header.dwSize
            );
        }
        default: break;
    }
    return DefWindowProcW(mWindowHandle, message, wParam, lParam);
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
    devices[0].hwndTarget = mWindowHandle;

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
    devices[1].hwndTarget = mWindowHandle;

    // adds gamepads
    devices[2].usUsagePage = 0x01;
    devices[2].usUsage = 0x05;
    devices[2].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    devices[2].hwndTarget = mWindowHandle;

    // note that this registration affects the entire application
    USAGI_WIN32_CHECK_THROW(
        RegisterRawInputDevices,
        devices.data(),
        static_cast<UINT>(devices.size()),
        sizeof(RAWINPUTDEVICE)
    );
}
}
