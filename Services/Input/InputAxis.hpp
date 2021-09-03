#pragma once

#include <cstdint>

namespace usagi
{
/**
 * \brief Names of keys, buttons, and axis on supported devices, including
 * standard 101 keyboard, mouse, and gamepad.
 */
enum class InputAxis : std::uint16_t
{
    UNKNOWN = 0,

    // ========================= Keyboard Codes ========================= //

    // FUNCTION KEYS

    ESCAPE,
    F1, F2, F3, F4,
    F5, F6, F7, F8,
    F9, F10, F11, F12,
    PRINT_SCREEN,
    SCROLL_LOCK,
    PAUSE,

    // ALPHANUMERIC KEYS

    GRAVE_ACCENT,
    DIGIT_0, DIGIT_1, DIGIT_2, DIGIT_3,
    DIGIT_4, DIGIT_5, DIGIT_6, DIGIT_7,
    DIGIT_8, DIGIT_9,
    A, B, C, D, E, F, G, H, I, J,
    K, L, M, N, O, P, Q, R, S, T,
    U, V, W, X, Y, Z,
    MINUS,
    EQUAL,
    LEFT_BRACKET, RIGHT_BRACKET,
    BACKSLASH,
    SEMICOLON,
    QUOTE, COMMA, PERIOD, SLASH,

    // FUNCTIONAL KEYS

    SPACE,
    BACKSPACE,
    TAB,
    CAPS_LOCK,
    ENTER,
    LEFT_SHIFT, RIGHT_SHIFT,
    LEFT_CONTROL, RIGHT_CONTROL,
    LEFT_ALT, RIGHT_ALT,
    LEFT_OS, RIGHT_OS,
    CONTEXT_MENU,

    // CONTROL PAD KEYS

    INSERT, DELETE,
    HOME, END,
    PAGE_UP, PAGE_DOWN,

    // ARROW PAD KEYS

    UP, DOWN, LEFT, RIGHT,

    // NUMPAD KEYS

    NUM_LOCK,
    NUM_DIVIDE, NUM_MULTIPLY, NUM_SUBTRACT, NUM_ADD,
    NUM_0, NUM_1, NUM_2, NUM_3,
    NUM_4, NUM_5, NUM_6, NUM_7,
    NUM_8, NUM_9,
    NUM_DECIMAL,
    NUM_ENTER,

    // ====================== Mouse Button Codes ======================== //

    MOUSE_CURSOR,

    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,

    // note: BUTTON 1-3 are aliases to LEFT/MIDDLE/RIGHT in Windows

    MOUSE_BUTTON_4,
    MOUSE_BUTTON_5,

    MOUSE_WHEEL_X,
    MOUSE_WHEEL_Y,

    // ===================== Gamepad Button Codes ======================= //

    PAD_LEFT_UP,
    PAD_LEFT_DOWN,
    PAD_LEFT_LEFT,
    PAD_LEFT_RIGHT,

    // Y/triangle
    PAD_RIGHT_UP,
    // A/cross
    PAD_RIGHT_DOWN,
    // X/square
    PAD_RIGHT_LEFT,
    // B/circle
    PAD_RIGHT_RIGHT,

    PAD_LEFT_SHOULDER,
    PAD_RIGHT_SHOULDER,

    PAD_LEFT_STICK,
    PAD_RIGHT_STICK,

    // back/share
    PAD_SETTING_LEFT,
    // start/option
    PAD_SETTING_RIGHT,

    ENUM_END,
};

const char * to_string(InputAxis key);
}
