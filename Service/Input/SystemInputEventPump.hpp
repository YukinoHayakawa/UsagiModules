#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service.hpp>

#include "InputEventInserter.hpp"
#include "ServiceInputSource.hpp"

namespace usagi
{
struct SystemInputEventPump
{
    using WriteAccess = ArchetypeInputEvent::ComponentFilterT;
    using ReadAccess = C<>;

    ArchetypeInputEvent archetype;

    template <typename RuntimeServices, typename EntityDatabaseAccess>
    auto update(RuntimeServices &&rt, EntityDatabaseAccess &&db)
    {
        auto &source = USAGI_SERVICE(rt, ServiceInputSource);

        class Inserter : public InputEventInserter
        {
            ArchetypeInputEvent &mArchetype;
            EntityDatabaseAccess mAccess;

        public:
            Inserter(
                ArchetypeInputEvent &archetype,
                EntityDatabaseAccess access)
                : mArchetype(archetype)
                , mAccess(access)
            {
            }

            ArchetypeInputEvent & archetype() override
            {
                return mArchetype;
            }

            void insert() override
            {
                mAccess.insert(mArchetype);
            }
        } visitor { archetype, db };

        while(source.pump_event(visitor));
    }
};
}
