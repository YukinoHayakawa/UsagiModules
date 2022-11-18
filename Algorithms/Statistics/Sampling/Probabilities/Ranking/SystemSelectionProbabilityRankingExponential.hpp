#pragma once

#include <numbers>

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>
#include <Usagi/Modules/Common/Indexing/EntityIndexDescriptor.hpp>
#include <Usagi/Modules/Common/Indexing/ServiceExternalEntityIndex.hpp>

namespace usagi
{
/**
 * \brief Assign exponentially adjusted ranking based selection probability
 * to entities. The total of probabilities will be normalized to 1.
 * \tparam IndexDescriptor The index used to determine the order of entities.
 * The ranking will be mapped to indexed entities staring from 1, assuming
 * that worse the fitness has lower the ranking.
 * \tparam TargetProbabilityComponent The component where the probability
 * should be written to. It must has an arithmetic field called `probability`.
 */
template <
    typename IndexDescriptor,
    Component TargetProbabilityComponent
>
struct SystemSelectionProbabilityRankingExponential
{
    using WriteAccess = C<TargetProbabilityComponent>;
    using ReadAccess = EntityIndexReadAccess<IndexDescriptor>;

    using ServiceAccessT = ServiceAccess<
        ServiceExternalEntityIndex<IndexDescriptor>
    >;

    /**
     * \brief
     *   In[28]:= c[n_] = Sum[1 - E^-i, {i, 1, n}] // Simplify
     *   Out[28]= (1 - E^-n + n - E n)/(1 - E)
     *   bug: this numerically calculated value may cause the sum of probabilities to deviate a little bit from 1. either divide the probabilities by their accumulation or don't do the normalization? 
     * \param n Number of candidate parents.
     * \return Normalizing constant used in exponential probability scaling.
     */
    static double normalizing_constant(const std::size_t n)
    {
        constexpr auto e = std::numbers::e;
        const double x = static_cast<double>(n);
        const double c = (1.0 - std::exp(-x) + x - e * x) / (1.0 - e);
        assert(c != 0.0);
        assert(!isnan(c));
        assert(!isinf(c));
        return c;
    }

    void update(ServiceAccessT rt, auto &&db)
    {
        auto &index = rt.entity_index(IndexDescriptor());
        const auto c = normalizing_constant(index.index.size());

        // todo: support multithreading
        for(std::size_t order = 1; auto &&[sort_key, entity_id] : index.index)
        {
            auto view = db.entity(entity_id);
            auto &prob = view.add_component(C<TargetProbabilityComponent>());
            prob.probability = (1.0 - std::exp(-static_cast<double>(order))) / c;
            assert(!isnan(prob.probability));
            ++order;
        }
    }
};
}
