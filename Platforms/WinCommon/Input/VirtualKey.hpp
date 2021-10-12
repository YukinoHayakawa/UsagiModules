#pragma once

#include <Usagi/Modules/IO/Input/InputAxis.hpp>

struct tagRAWKEYBOARD;

namespace usagi::win32
{
InputAxis translate_keycode(const tagRAWKEYBOARD &keyboard);
}
