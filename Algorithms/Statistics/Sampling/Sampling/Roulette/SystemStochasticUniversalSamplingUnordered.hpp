#pragma once

#include <range/v3/numeric/accumulate.hpp>

#include <Usagi/Entity/EntityDatabase.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>
#include <Usagi/Modules/Algorithms/Statistics/RandomNumbers/ServiceRandomNumberGenerator.hpp>
#include <Usagi/Modules/Runtime/KeyValueStorage/ServiceRuntimeKeyValueStorage.hpp>

namespace usagi
{
/**
 * \brief Pick samples by rotating a spin wheel with equal spaced pointers.
 * \tparam Query The entity query defining the sample space.
 * \tparam ProbabilityDistribution The component storing the probability
 * distribution values.
 * \tparam SampleArchetype The archetype type used to store entity samples.
 * \tparam SampleIdentity The component storing the EntityId of samples.
 */
template <
    SimpleComponentQuery Query,
    Component ProbabilityDistribution,
    typename SampleArchetype,
    Component SampleIdentity
>
requires QueryIncludeComponent<Query, ProbabilityDistribution> &&
    ArchetypeHasComponent<SampleArchetype, SampleIdentity>
struct SystemStochasticUniversalSamplingUnordered
{
    using WriteAccess = typename SampleArchetype::ComponentFilterT;
    using ReadAccess = typename Query::ReadAccess;

    using ServiceAccessT = ServiceAccess<
        ServiceRuntimeKeyValueStorage,
        ServiceThreadLocalRNG
    >;

    SampleArchetype sample_archetype;

    void update(ServiceAccessT rt, auto &&db)
    {
        const auto target_sample_size =
            rt.kv_storage().require<std::size_t>("target_sample_size");

        using ProbabilityT = std::remove_cvref_t<
            decltype(std::declval<ProbabilityDistribution>().probability)
        >;

        // Introduction to Evolutionary Computing p.84
        //
        // WHILE ( current member ≤ λ ) DO
        //     WHILE ( r ≤ a[i] ) DO
        //         set mating pool[current member] = parents[i];
        //         set r = r + 1/λ;
        //         set current member = current member + 1;
        //     OD
        //     set i = i + 1;
        // OD

        // calculate spin wheel size
        // i.e. the integral of probability density function
        // or say the max value of cumulative probability distribution
        // this impl only requires that all p are non-negative. it does not
        // require that the sum of PDF to be 1.
        const ProbabilityT cpd_max =
            ranges::accumulate(
                db.view(Query()), ProbabilityT { }, ranges::plus(), 
                [](auto &&e) {
                    const auto p = e(C<ProbabilityDistribution>()).probability;
                    assert(!isnan(p));
                    assert(!isinf(p));
                    assert(p >= 0);
                    return p;
                }
            );

        // todo normalized probabilities cause precision issues
        // todo: can the pointer finally go past the end of the wheel?
        // since std::uniform_real_distribution generates samples on a
        // [,) range, so in principle multiple added samples won't get to
        // cpd_max;

        const ProbabilityT pointer_interval = cpd_max / target_sample_size;
        ProbabilityT pointer_position;
        // pick the starting position of the spin wheel pointer from
        // [0, cpd_max/target_sample_size). (the book uses a closed range
        // though).
        {
            std::uniform_real_distribution<ProbabilityT> dist {
                0, pointer_interval
            };
            // and use it to set the starting pointer position randomly
            pointer_position = dist(rt.thread_rng());
        }

        // group of entities to draw samples from
        auto range = db.view(Query());
        // value of cumulative probability distribution wrt positions in the
        // entity range
        ProbabilityT accumulated_p { };
        // number of sample drawn up to now
        std::size_t current_sample_size = 0;

        // iterate over the sample space of candidate individuals
        // note that one individual might be chosen as parent for multiple times
        for(auto iter = range.begin(); iter != range.end(); ++iter)
        {
            auto entity = *iter;

            // update cumulative selection probability to the current
            // individual
            accumulated_p += entity(C<ProbabilityDistribution>()).probability;

            // compare the pointer position against CPD value. add the current
            // individual to the samples until that the pointer position
            // go pasts the current CPD value, in which case move on to the
            // next individual.
            while(pointer_position < accumulated_p)
            {
                sample_archetype(C<SampleIdentity>()).id = entity.id();
                db.insert(sample_archetype);
                ++current_sample_size;
                pointer_position += pointer_interval;
            }
        }
        assert(current_sample_size == target_sample_size);
    }
};
}
