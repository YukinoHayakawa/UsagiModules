#pragma once

#include <Usagi/Entity/Archetype.hpp>
#include <Usagi/Module/Common/Time/ComponentTimestamp.hpp>

namespace usagi
{
using ArchetypeInputEvent = Archetype<
    ComponentTimestamp
>;

USAGI_DECL_TAG_COMPONENT(TagKeyPressed);
}
