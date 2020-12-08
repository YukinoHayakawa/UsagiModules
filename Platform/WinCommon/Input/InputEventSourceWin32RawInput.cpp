#include "InputEventSourceWin32RawInput.hpp"

#include <array>
#include <vector>

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>
#include <Usagi/Module/Service/Input/InputEventVisitor.hpp>
#include <Usagi/Module/Service/Input/Keyboard/KeyCode.hpp>

namespace usagi
{
namespace
{
InputEventVisitor *gEventVisitor = nullptr;
}

namespace detail::win32_input
{
LRESULT CALLBACK input_message_dispatcher(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct RawInputSink
{
    static inline const wchar_t WINDOW_CLASS_NAME[] = L"UsagiWin32InputSink";
    HINSTANCE process_instance_handle = GetModuleHandleW(nullptr);
    HWND message_window = nullptr;

    void register_window_class();
    void create_input_sink_window();
    void register_raw_input_devices();

    void unregister_window_class();
    void destroy_input_sink_window();

    RawInputSink()
    {
        register_window_class();
        create_input_sink_window();
        register_raw_input_devices();
    }

    ~RawInputSink()
    {
        destroy_input_sink_window();
        unregister_window_class();
    }
};

void RawInputSink::register_window_class()
{
    WNDCLASSEXW wcex { };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = &input_message_dispatcher;
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

KeyCode translate_keycode(const RAWKEYBOARD *keyboard)
{
    switch(keyboard->VKey)
    {
    case VK_BACK:
        return KeyCode::BACKSPACE;
    case VK_TAB:
        return KeyCode::TAB;
    case VK_PAUSE:
        return KeyCode::PAUSE;
    case VK_CAPITAL:
        return KeyCode::CAPS_LOCK;
    case VK_ESCAPE:
        return KeyCode::ESCAPE;
    case VK_SPACE:
        return KeyCode::SPACE;
    case VK_PRIOR:
        return KeyCode::PAGE_UP;
    case VK_NEXT:
        return KeyCode::PAGE_DOWN;
    case VK_END:
        return KeyCode::END;
    case VK_HOME:
        return KeyCode::HOME;
    case VK_LEFT:
        return KeyCode::LEFT;
    case VK_UP:
        return KeyCode::UP;
    case VK_RIGHT:
        return KeyCode::RIGHT;
    case VK_DOWN:
        return KeyCode::DOWN;
    case VK_SNAPSHOT:
        return KeyCode::PRINT_SCREEN;
    case VK_INSERT:
        return KeyCode::INSERT;
    case VK_DELETE:
        return KeyCode::DELETE;
    case '0':
        return KeyCode::DIGIT_0;
    case '1':
        return KeyCode::DIGIT_1;
    case '2':
        return KeyCode::DIGIT_2;
    case '3':
        return KeyCode::DIGIT_3;
    case '4':
        return KeyCode::DIGIT_4;
    case '5':
        return KeyCode::DIGIT_5;
    case '6':
        return KeyCode::DIGIT_6;
    case '7':
        return KeyCode::DIGIT_7;
    case '8':
        return KeyCode::DIGIT_8;
    case '9':
        return KeyCode::DIGIT_9;
    case 'A':
        return KeyCode::A;
    case 'B':
        return KeyCode::B;
    case 'C':
        return KeyCode::C;
    case 'D':
        return KeyCode::D;
    case 'E':
        return KeyCode::E;
    case 'F':
        return KeyCode::F;
    case 'G':
        return KeyCode::G;
    case 'H':
        return KeyCode::H;
    case 'I':
        return KeyCode::I;
    case 'J':
        return KeyCode::J;
    case 'K':
        return KeyCode::K;
    case 'L':
        return KeyCode::L;
    case 'M':
        return KeyCode::M;
    case 'N':
        return KeyCode::N;
    case 'O':
        return KeyCode::O;
    case 'P':
        return KeyCode::P;
    case 'Q':
        return KeyCode::Q;
    case 'R':
        return KeyCode::R;
    case 'S':
        return KeyCode::S;
    case 'T':
        return KeyCode::T;
    case 'U':
        return KeyCode::U;
    case 'V':
        return KeyCode::V;
    case 'W':
        return KeyCode::W;
    case 'X':
        return KeyCode::X;
    case 'Y':
        return KeyCode::Y;
    case 'Z':
        return KeyCode::Z;
    case VK_LWIN:
        return KeyCode::LEFT_OS;
    case VK_RWIN:
        return KeyCode::RIGHT_OS;
    case VK_APPS:
        return KeyCode::CONTEXT_MENU;
    case VK_NUMPAD0:
        return KeyCode::NUM_0;
    case VK_NUMPAD1:
        return KeyCode::NUM_1;
    case VK_NUMPAD2:
        return KeyCode::NUM_2;
    case VK_NUMPAD3:
        return KeyCode::NUM_3;
    case VK_NUMPAD4:
        return KeyCode::NUM_4;
    case VK_NUMPAD5:
        return KeyCode::NUM_5;
    case VK_NUMPAD6:
        return KeyCode::NUM_6;
    case VK_NUMPAD7:
        return KeyCode::NUM_7;
    case VK_NUMPAD8:
        return KeyCode::NUM_8;
    case VK_NUMPAD9:
        return KeyCode::NUM_9;
    case VK_MULTIPLY:
        return KeyCode::NUM_MULTIPLY;
    case VK_ADD:
        return KeyCode::NUM_ADD;
    case VK_SUBTRACT:
        return KeyCode::NUM_SUBTRACT;
    case VK_DECIMAL:
        return KeyCode::NUM_DECIMAL;
    case VK_DIVIDE:
        return KeyCode::NUM_DIVIDE;
    case VK_F1:
        return KeyCode::F1;
    case VK_F2:
        return KeyCode::F2;
    case VK_F3:
        return KeyCode::F3;
    case VK_F4:
        return KeyCode::F4;
    case VK_F5:
        return KeyCode::F5;
    case VK_F6:
        return KeyCode::F6;
    case VK_F7:
        return KeyCode::F7;
    case VK_F8:
        return KeyCode::F8;
    case VK_F9:
        return KeyCode::F9;
    case VK_F10:
        return KeyCode::F10;
    case VK_F11:
        return KeyCode::F11;
    case VK_F12:
        return KeyCode::F12;
    case VK_NUMLOCK:
        return KeyCode::NUM_LOCK;
    case VK_SCROLL:
        return KeyCode::SCROLL_LOCK;
    case VK_OEM_1:
        return KeyCode::SEMICOLON;
    case VK_OEM_PLUS:
        return KeyCode::EQUAL;
    case VK_OEM_COMMA:
        return KeyCode::COMMA;
    case VK_OEM_MINUS:
        return KeyCode::MINUS;
    case VK_OEM_PERIOD:
        return KeyCode::PERIOD;
    case VK_OEM_2:
        return KeyCode::SLASH;
    case VK_OEM_3:
        return KeyCode::GRAVE_ACCENT;
    case VK_OEM_4:
        return KeyCode::LEFT_BRACKET;
    case VK_OEM_5:
        return KeyCode::BACKSLASH;
    case VK_OEM_6:
        return KeyCode::RIGHT_BRACKET;
    case VK_OEM_7:
        return KeyCode::QUOTE;
    default:
        break;
    }
    const auto e0_prefixed = (keyboard->Flags & RI_KEY_E0) != 0;
    switch(keyboard->VKey)
    {
    case VK_SHIFT:
        return MapVirtualKeyW(keyboard->MakeCode, MAPVK_VSC_TO_VK_EX)
            == VK_LSHIFT
            ? KeyCode::LEFT_SHIFT
            : KeyCode::RIGHT_SHIFT;
    case VK_CONTROL:
        return e0_prefixed ? KeyCode::RIGHT_CONTROL : KeyCode::LEFT_CONTROL;
    case VK_MENU:
        return e0_prefixed ? KeyCode::RIGHT_ALT : KeyCode::LEFT_ALT;
    case VK_RETURN:
        return e0_prefixed ? KeyCode::NUM_ENTER : KeyCode::ENTER;
    default:
        return KeyCode::UNKNOWN;
    }
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
            assert(gEventVisitor);

            const auto input = get_raw_input_data(lParam);
            switch(input->header.dwType)
            {
                case RIM_TYPEKEYBOARD:
                {
                    auto &kb = input->data.keyboard;
                    const auto key = translate_keycode(&kb);
                    // ignore keys other than those on 101 keyboard
                    if(key == KeyCode::UNKNOWN)
                        break;
                    const auto pressed = (kb.Flags & RI_KEY_BREAK) == 0;

                    ArchetypeKeyboardEvent event;
                    // todo: timestamp
                    event.val<ComponentKeyCode>().code = key;
                    gEventVisitor->keyboard_event(event, pressed);

                    break;
                }

                case RIM_TYPEMOUSE:
                    // todo
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

namespace
{
using namespace detail::win32_input;
std::weak_ptr<RawInputSink> gRawInputSink;
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

void InputEventSourceWin32RawInput::pump_events(InputEventVisitor &visitor)
{
    gEventVisitor = &visitor;
    MSG msg;
    while(PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    gEventVisitor = nullptr;
}
}
