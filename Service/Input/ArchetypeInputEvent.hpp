#pragma once

#include <Usagi/Entity/Archetype.hpp>
#include <Usagi/Module/Common/Time/ComponentTimestamp.hpp>

#include "ComponentInputEvent.hpp"

namespace usagi
{
using ArchetypeInputEvent = Archetype<
    ComponentTimestamp,
    ComponentInputEvent
>;

USAGI_DECL_TAG_COMPONENT(TagEventProcessed);
}
