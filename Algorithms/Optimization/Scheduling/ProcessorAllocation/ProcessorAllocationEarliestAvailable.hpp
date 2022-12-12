#pragma once

#include <type_traits>

#include <Usagi/Library/Utilities/Functional.hpp>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleMixinNodeVisitorAvailableProcessors.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
struct ProcessorAllocationEarliestAvailable
{
    template <typename Graph>
    auto operator()(Graph &&graph) requires
        // available_processors
        std::is_base_of_v<
            ScheduleMixinNodeVisitorAvailableProcessors<std::decay_t<Graph>>,
            std::decay_t<Graph>
        >
    {
        auto range = graph.available_processors();
        auto iter = std::ranges::min_element(
            range,
            std::ranges::less { },
            [](auto &&v) { return std::get<1>(v); }
            // operators::get_index<1> { }
        );
        USAGI_ASSERT_THROW(
            iter != range.end(), 
            std::runtime_error("no available processor?")
        );
        return *iter;
    }
};
}
