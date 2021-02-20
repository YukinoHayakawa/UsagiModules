#pragma once

#include <cstddef>

#include "ArchetypeKeyboardEvent.hpp"

namespace usagi
{
class Keyboard
{
    // Key states should be updated after an input event is consumed.
    bool mKeyStates[KeyCode::ENUM_END] { };

public:
    virtual ~Keyboard() = default;

    bool isKeyPressed(const KeyCode key) const
    {
        return mKeyStates[static_cast<std::size_t>(key)];
    }

    /**
     * \brief The keyboard implementation fills in the earliest happened event
     * in the queue into `event`, if any.
     * \param event
     * \return True if an event was filled into `event`. False otherwise.
     */
    virtual bool pump_event(ArchetypeKeyboardEvent &event) = 0;

    void consume_event()// ????????? howto
};
}
