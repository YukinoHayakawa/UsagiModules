#include "Input.hpp"

#include <Usagi/Module/Platform/WinCommon/Win32.hpp>

namespace usagi::win32
{
bool raw_input__handle_keyboard(
    InputEventInserter &inserter,
    const tagRAWKEYBOARD &keyboard,
    const MessageInfo &info)
{
    const auto key = translate_keycode(keyboard);

    // ignore keys other than those on 101 keyboard
    if(key == InputAxis::UNKNOWN)
        return false;

    auto &archetype = inserter.archetype();

    // todo: timestamp

    auto &evt = archetype.component<ComponentInputEvent>();
    const auto pressed = (keyboard.Flags & RI_KEY_BREAK) == 0;
    evt.axis = key;
    evt.absolute = { 0, pressed };
    evt.relative = { 0, pressed ? 1 : -1 };

    inserter.insert();

    return true;
}
}
