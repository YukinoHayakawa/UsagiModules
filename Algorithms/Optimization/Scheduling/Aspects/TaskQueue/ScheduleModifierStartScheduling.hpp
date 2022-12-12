#pragma once

#include <type_traits>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecutionBarrier.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Precedence/ScheduleMixinTaskBarrierIndices.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleMixinRootNodeIndex.hpp>

namespace usagi
{
struct ScheduleModifierStartScheduling
{
    template <typename Graph>
    void operator()(Graph &&graph) requires
        std::is_base_of_v<
            ScheduleMixinRootNodeIndex<
                typename std::decay_t<Graph>::VertexIndexT>,
            std::decay_t<Graph>
        > && std::is_base_of_v<
            ScheduleMixinTaskBarrierIndices<
                typename std::decay_t<Graph>::TaskIndexT,
                typename std::decay_t<Graph>::VertexIndexT
            >,
            std::decay_t<Graph>
        >
    {
        using TaskIndexT = typename std::decay_t<Graph>::TaskIndexT;

        graph.template add_edge<
            ScheduleNodeRoot,
            ScheduleNodeExecutionBarrier<TaskIndexT>
        >(  graph.root_node_index, 
            graph.task_begin_barrier_index(graph.source_task_id())
        );
    }
};
}
