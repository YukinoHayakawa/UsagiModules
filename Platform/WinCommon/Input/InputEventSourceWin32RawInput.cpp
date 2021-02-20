#include "Input.hpp"
#include "InputEventSourceWin32RawInput.hpp"

#include <vector>

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>

#include "RawInputSink.hpp"

namespace usagi
{
namespace
{
InputEventInserter *gEventInserter = nullptr;
std::weak_ptr<RawInputSink> gRawInputSink;
}

namespace detail::win32_input
{
// bug not thread safe
RAWINPUT* get_raw_input_data(LPARAM lParam)
{
    static std::vector<BYTE> buffer;
    UINT dwSize;

    // fetch raw input data
    GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        nullptr,
        &dwSize,
        sizeof(RAWINPUTHEADER)
    );
    buffer.resize(dwSize);
    if(GetRawInputData(
        reinterpret_cast<HRAWINPUT>(lParam),
        RID_INPUT,
        buffer.data(),
        &dwSize,
        sizeof(RAWINPUTHEADER)
    ) != dwSize)
    {
        USAGI_WIN32_THROW("GetRawInputData");
    }

    return reinterpret_cast<RAWINPUT*>(buffer.data());
}

LRESULT input_message_dispatcher(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
        // unbuffered raw input data
        case WM_INPUT:
        {
            assert(gEventInserter);

            const auto input = get_raw_input_data(lParam);
            switch(input->header.dwType)
            {
                case RIM_TYPEKEYBOARD:
                    win32::raw_input_handle_keyboard(
                        *gEventInserter,
                        input->data.keyboard
                    );
                    break;

                case RIM_TYPEMOUSE:
                    win32::raw_input_handle_mouse(
                        *gEventInserter,
                        input->data.mouse
                    );
                    break;

                default:
                    break;
            }
            return 0;
        }
        // todo: handles device addition/removal
        case WM_INPUT_DEVICE_CHANGE:
        {
            break;
        }
        default: break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
}

InputEventSourceWin32RawInput::InputEventSourceWin32RawInput()
{
    mSink = gRawInputSink.lock();
    if(mSink == nullptr)
    {
        mSink = std::make_shared<RawInputSink>();
        gRawInputSink = mSink;
    }
}

bool InputEventSourceWin32RawInput::pump_event(InputEventInserter &inserter)
{
    gEventInserter = &inserter;
    MSG msg;
    const bool any_message = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE);
    if(any_message)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    gEventInserter = nullptr;
    return any_message;
}
}
