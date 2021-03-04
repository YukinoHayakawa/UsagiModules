#pragma once

#include <Usagi/Module/Service/Input/InputEventInserter.hpp>

struct tagRAWKEYBOARD;
struct tagRAWMOUSE;

namespace usagi::win32
{
InputAxis translate_keycode(const tagRAWKEYBOARD &keyboard);
bool raw_input__handle_keyboard(
    InputEventInserter &inserter,
    const tagRAWKEYBOARD &keyboard);
bool raw_input__handle_mouse(
    InputEventInserter &inserter,
    const tagRAWMOUSE &mouse);
}
