#pragma once

#include <range/v3/view/chunk.hpp>

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>

#include <Usagi/Modules/Algorithms/Optimization/Evolutionary/Services/ServiceRandomNumberGenerator.hpp>

namespace usagi
{
/**
 * \brief Apply chromosome operator to individuals that satisfy the specified
 * filters.
 * \tparam Chromosome Component type of the chromosome to be modified.
 * \tparam Operator Operation that will be applied to the chromosome.
 * \tparam IncludeFilter Extra include filter.
 * \tparam ExcludeFilter Extra exclude filter.
 */
template <
    Component Chromosome,
    typename Operator,
    IsComponentFilter IncludeFilter = C<>,
    IsComponentFilter ExcludeFilter = C<>
>
struct SystemApplyChromosomeOperator
{
    using WriteAccess = C<Chromosome>;
    using ReadAccess = FilterConcatenatedT<IncludeFilter, ExcludeFilter>;

    using ServiceAccessT = ServiceAccess<ServiceRandomNumberGenerator>;

    void update(ServiceAccessT rt, auto &&db)
    {
        for(auto &&e : db.view(
            FilterConcatenatedT<IncludeFilter, C<Chromosome>>(), 
            ExcludeFilter()))
        {
            // todo: apply to chunked views?
            auto &chromosome = USAGI_COMPONENT(e, Chromosome);
            Operator()(chromosome, rt.rng());
        }
    }
};
}
