#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>

namespace usagi
{
/**
 * \brief Visit entities that satisfies the provided query.
 * The query filter defined by it is also applied.
 * \tparam Query The query specifying the entity query filter.
 * \tparam Visitor The callable that will be applied to visited entities.
 * It must accept the entity view and the sorted key defined by the index
 * descriptor as arguments.
 *
 * todo: thread safety
 */
template <
    SimpleComponentQuery Query,
    typename Visitor
>
struct SystemVisitEntities
{
    using WriteAccess = typename Visitor::WriteAccess;
    using ReadAccess = typename Query::ReadAccess;

    // declare access to entity index
    using ServiceAccessT = ServiceAccess<>;

    void update(ServiceAccessT rt, auto &&db)
    {
        Visitor visitor;

        // todo: multithreading
        for(auto &&entity_view : db.view(Query()))
        {
            visitor(entity_view);
        }
    }
};
}
