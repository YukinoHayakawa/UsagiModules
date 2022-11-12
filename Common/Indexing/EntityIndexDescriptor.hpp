#pragma once

#include <map>
#include <string>

#include <Usagi/Entity/ComponentQueryFilter.hpp>
#include <Usagi/Entity/detail/EntityId.hpp>

namespace usagi
{
/**
 * \brief Describe an Entity Index that will be maintained by the engine.
 * \tparam Query Include and exclude filters.
 * \tparam SortKey Component that will be used to sort the keys. SortKey must
 * be included in Query::IncludeFilter.
 * \tparam Projection Function that maps the component to a value to be accepted
 * by the comparator.
 * \tparam Comparator The comparator which gives a total order of the projected
 * values.
 * \tparam SortedValueContainer The container used to store the 
 */
template <
    SimpleComponentQuery Query,
    Component SortKey,
    typename Projection,
    template <typename Key>
    typename Comparator,
    template <typename Key, typename Value, typename Comp, typename...>
    typename OrderedContainer = std::map
>
// requires
//     /*std::totally_ordered_with<Projection, Comparator<
//         decltype(std::declval<Projection>()(std::declval<SortKey>()))
//     >>
//  && */typename Query::IncludeFilter::template HAS_COMPONENT<SortKey>
struct EntityIndexDescriptor
{
    using SortKey = SortKey;

    // projected value used for sorting entities
    using ProjectedKey = std::invoke_result_t<Projection, SortKey>;

    // template <typename Key, typename Value, typename Comp>
    // using Container = OrderedContainer;

    using SortedKeyContainer = OrderedContainer<
        ProjectedKey,
        EntityId,
        Comparator<ProjectedKey>
    >;

    // note that in order to use aggregates the ProjectedKey must be
    // accumulable.
    using AggregateContainer = std::map<std::string, ProjectedKey>;

    using Query = Query;
    using IncludeFilter = typename Query::IncludeFilter;
    using ExcludeFilter = typename Query::ExcludeFilter;
    using SortedComponent = SortKey;
    using Projection = Projection;
    // using Comparator = Comparator;
};

template <typename T>
using EntityQueryReadAccess = typename T::Query::ReadAccess;
}
