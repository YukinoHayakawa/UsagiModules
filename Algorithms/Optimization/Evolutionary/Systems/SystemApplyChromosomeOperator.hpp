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
 * \tparam Chromosome Component type of the chromosome to be modified. The type
 * must be convertible to a range.
 * \tparam Operator Operation that will be applied to the chromosome.
 * \tparam ChunkSize When not 0, the chromosome will be chunked into subviews
 * according to the specified size and the operator will be applied to each
 * subrange independently.
 * \tparam IncludeFilter Extra include filter.
 * \tparam ExcludeFilter Extra exclude filter.
 */
template <
    Component Chromosome,
    typename Operator,
    std::size_t ChunkSize = 0,
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
            auto &chromosome = USAGI_COMPONENT(e, Chromosome);

            if constexpr(ChunkSize == 0)
            {
                Operator()(chromosome, rt.rng());
            }
            else
            {
                for(auto &&chunked : 
                    chromosome | ranges::views::chunk(ChunkSize))
                {
                    Operator()(chunked, rt.rng());
                }
            }
        }
    }
};
}
