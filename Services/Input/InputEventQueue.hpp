#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>

#include "ArchetypeInputEvent.hpp"

namespace usagi
{
using InputEventQueue = EntityDatabase<
    entity::EntityDatabaseConfiguration<>,
    ComponentTimestamp,
    ComponentInputEvent
>;
}
