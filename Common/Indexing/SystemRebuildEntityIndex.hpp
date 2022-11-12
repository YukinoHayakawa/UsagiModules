#pragma once

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>

#include "ServiceExternalEntityIndex.hpp"

namespace usagi
{
template <
    typename IndexDescriptor
// todo: visit dirty entities only
>
struct SystemRebuildEntityIndex
{
    using WriteAccess = C<>;
    using ReadAccess = C<>;

    using ServiceAccessT = ServiceAccess<
        ServiceExternalEntityIndex<IndexDescriptor>
    >;

    void update(ServiceAccessT rt, auto &&db)
    {
        auto &index = rt.entity_index(IndexDescriptor());
        index.reset_index();

        for(auto &&e : db.view(typename IndexDescriptor::Query()))
        {
            index.visit(e);
        }
    }
};
}
