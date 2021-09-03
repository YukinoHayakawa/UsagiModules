#pragma once

#include <Usagi/Modules/Services/Input/InputAxis.hpp>

struct tagRAWKEYBOARD;

namespace usagi::win32
{
InputAxis translate_keycode(const tagRAWKEYBOARD &keyboard);
}
