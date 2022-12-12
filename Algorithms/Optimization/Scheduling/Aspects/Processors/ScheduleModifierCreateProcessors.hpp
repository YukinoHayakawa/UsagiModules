#pragma once

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleNodeRoot.hpp>

#include "ScheduleNodeProcessorReady.hpp"

namespace usagi
{
// create initial processor resources
struct ScheduleModifierCreateProcessors
{
    template <typename Graph, typename ProcIndexT>
    void operator()(Graph &&graph, ProcIndexT num_proc) requires
        std::is_base_of_v<
            ScheduleMixinRootNodeIndex<
                typename std::decay_t<Graph>::VertexIndexT>,
            std::decay_t<Graph>
        >
    {
        for(ProcIndexT i = 0; i < num_proc; ++i)
        {
            auto [index, proc] = graph.template add_vertex<
                ScheduleNodeProcessorReady
            >();

            proc.processor_index = i;
            proc.ready_time = graph.root_node_ptr->ready_time;

            graph.template add_edge<
                ScheduleNodeRoot,
                ScheduleNodeProcessorReady
            >(graph.root_node_index, index);
        }
    }
};
}
