#include "Input.hpp"

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>

namespace usagi::win32
{
// Ref:
// https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/
InputAxis translate_keycode(const tagRAWKEYBOARD &keyboard)
{
    switch(keyboard.VKey)
    {
        case VK_BACK: return InputAxis::BACKSPACE;
        case VK_TAB: return InputAxis::TAB;
        case VK_PAUSE: return InputAxis::PAUSE;
        case VK_CAPITAL: return InputAxis::CAPS_LOCK;
        case VK_ESCAPE: return InputAxis::ESCAPE;
        case VK_SPACE: return InputAxis::SPACE;
        case VK_PRIOR: return InputAxis::PAGE_UP;
        case VK_NEXT: return InputAxis::PAGE_DOWN;
        case VK_END: return InputAxis::END;
        case VK_HOME: return InputAxis::HOME;
        case VK_LEFT: return InputAxis::LEFT;
        case VK_UP: return InputAxis::UP;
        case VK_RIGHT: return InputAxis::RIGHT;
        case VK_DOWN: return InputAxis::DOWN;
        case VK_SNAPSHOT: return InputAxis::PRINT_SCREEN;
        case VK_INSERT: return InputAxis::INSERT;
        case VK_DELETE: return InputAxis::DELETE;
        case '0': return InputAxis::DIGIT_0;
        case '1': return InputAxis::DIGIT_1;
        case '2': return InputAxis::DIGIT_2;
        case '3': return InputAxis::DIGIT_3;
        case '4': return InputAxis::DIGIT_4;
        case '5': return InputAxis::DIGIT_5;
        case '6': return InputAxis::DIGIT_6;
        case '7': return InputAxis::DIGIT_7;
        case '8': return InputAxis::DIGIT_8;
        case '9': return InputAxis::DIGIT_9;
        case 'A': return InputAxis::A;
        case 'B': return InputAxis::B;
        case 'C': return InputAxis::C;
        case 'D': return InputAxis::D;
        case 'E': return InputAxis::E;
        case 'F': return InputAxis::F;
        case 'G': return InputAxis::G;
        case 'H': return InputAxis::H;
        case 'I': return InputAxis::I;
        case 'J': return InputAxis::J;
        case 'K': return InputAxis::K;
        case 'L': return InputAxis::L;
        case 'M': return InputAxis::M;
        case 'N': return InputAxis::N;
        case 'O': return InputAxis::O;
        case 'P': return InputAxis::P;
        case 'Q': return InputAxis::Q;
        case 'R': return InputAxis::R;
        case 'S': return InputAxis::S;
        case 'T': return InputAxis::T;
        case 'U': return InputAxis::U;
        case 'V': return InputAxis::V;
        case 'W': return InputAxis::W;
        case 'X': return InputAxis::X;
        case 'Y': return InputAxis::Y;
        case 'Z': return InputAxis::Z;
        case VK_LWIN: return InputAxis::LEFT_OS;
        case VK_RWIN: return InputAxis::RIGHT_OS;
        case VK_APPS: return InputAxis::CONTEXT_MENU;
        case VK_NUMPAD0: return InputAxis::NUM_0;
        case VK_NUMPAD1: return InputAxis::NUM_1;
        case VK_NUMPAD2: return InputAxis::NUM_2;
        case VK_NUMPAD3: return InputAxis::NUM_3;
        case VK_NUMPAD4: return InputAxis::NUM_4;
        case VK_NUMPAD5: return InputAxis::NUM_5;
        case VK_NUMPAD6: return InputAxis::NUM_6;
        case VK_NUMPAD7: return InputAxis::NUM_7;
        case VK_NUMPAD8: return InputAxis::NUM_8;
        case VK_NUMPAD9: return InputAxis::NUM_9;
        case VK_MULTIPLY: return InputAxis::NUM_MULTIPLY;
        case VK_ADD: return InputAxis::NUM_ADD;
        case VK_SUBTRACT: return InputAxis::NUM_SUBTRACT;
        case VK_DECIMAL: return InputAxis::NUM_DECIMAL;
        case VK_DIVIDE: return InputAxis::NUM_DIVIDE;
        case VK_F1: return InputAxis::F1;
        case VK_F2: return InputAxis::F2;
        case VK_F3: return InputAxis::F3;
        case VK_F4: return InputAxis::F4;
        case VK_F5: return InputAxis::F5;
        case VK_F6: return InputAxis::F6;
        case VK_F7: return InputAxis::F7;
        case VK_F8: return InputAxis::F8;
        case VK_F9: return InputAxis::F9;
        case VK_F10: return InputAxis::F10;
        case VK_F11: return InputAxis::F11;
        case VK_F12: return InputAxis::F12;
        case VK_NUMLOCK: return InputAxis::NUM_LOCK;
        case VK_SCROLL: return InputAxis::SCROLL_LOCK;
        case VK_OEM_1: return InputAxis::SEMICOLON;
        case VK_OEM_PLUS: return InputAxis::EQUAL;
        case VK_OEM_COMMA: return InputAxis::COMMA;
        case VK_OEM_MINUS: return InputAxis::MINUS;
        case VK_OEM_PERIOD: return InputAxis::PERIOD;
        case VK_OEM_2: return InputAxis::SLASH;
        case VK_OEM_3: return InputAxis::GRAVE_ACCENT;
        case VK_OEM_4: return InputAxis::LEFT_BRACKET;
        case VK_OEM_5: return InputAxis::BACKSLASH;
        case VK_OEM_6: return InputAxis::RIGHT_BRACKET;
        case VK_OEM_7: return InputAxis::QUOTE;
        default: break;
    }
    const auto e0_prefixed = (keyboard.Flags & RI_KEY_E0) != 0;
    switch(keyboard.VKey)
    {
        case VK_SHIFT:
            return MapVirtualKeyW(
                keyboard.MakeCode, MAPVK_VSC_TO_VK_EX
            ) == VK_LSHIFT
                ? InputAxis::LEFT_SHIFT : InputAxis::RIGHT_SHIFT;
        case VK_CONTROL:
            return e0_prefixed
                ? InputAxis::RIGHT_CONTROL : InputAxis::LEFT_CONTROL;
        case VK_MENU:
            return e0_prefixed
                ? InputAxis::RIGHT_ALT : InputAxis::LEFT_ALT;
        case VK_RETURN:
            return e0_prefixed
                ? InputAxis::NUM_ENTER : InputAxis::ENTER;
        default: return InputAxis::UNKNOWN;
    }
}
}
