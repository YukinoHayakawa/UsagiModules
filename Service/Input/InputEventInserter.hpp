#pragma once

#include "ArchetypeInputEvent.hpp"

namespace usagi
{
class InputEventInserter
{
public:
    virtual ~InputEventInserter() = default;

    virtual ArchetypeInputEvent & archetype() = 0;
    virtual void insert() = 0;
};
}
