#pragma once

#include <type_traits>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecutionBarrier.hpp>

#include "ScheduleMixinTaskBarrierIndices.hpp"

namespace usagi
{
struct ScheduleModifierInsertPrecedenceConstraints
{
    template <typename Graph>
    void operator()(Graph &&graph) requires
        // task_begin_barrier_index & task_end_barrier_index
        std::is_base_of_v<
            ScheduleMixinTaskBarrierIndices<
                typename std::decay_t<Graph>::TaskIndexT,
                typename std::decay_t<Graph>::VertexIndexT
            >,
            std::decay_t<Graph>
        >
    {
        // using VertexIndexT = typename std::decay_t<Graph>::VertexIndexT;
        using TaskIndexT = typename std::decay_t<Graph>::TaskIndexT;

        using Barrier = ScheduleNodeExecutionBarrier<TaskIndexT>;

        // insert begin and end barriers of tasks. the actual execution
        // nodes should be inserted by other modifiers (subtask, etc).
        // if no execution node is inserted between the begin and end barriers
        // the correct execution graph won't be formed.
        for(TaskIndexT i = 0; i < graph.num_tasks(); ++i)
        {
            // add the beginning barrier
            auto [begin_idx, begin_ref] = graph.template add_vertex<Barrier>();
            graph.task_begin_barrier_indices[i] = begin_idx;

            // add the finishing barrier
            auto [end_idx, end_ref] = graph.template add_vertex<Barrier>();
            // waiting number should be handled by
            // ScheduleEventHandlerPrecedenceConstraints
            // end_ref.num_waiting_input = num_subtasks;
            graph.task_end_barrier_indices[i] = end_idx;
        }

        // add precedence constraints
        for(TaskIndexT i = 0; i < graph.num_tasks(); ++i)
        {
            for(auto &&out : graph.descendant_tasks(i))
            {
                graph.template add_edge<Barrier, Barrier>(
                    graph.task_end_barrier_index(i),
                    graph.task_begin_barrier_index(out)
                );
            }
        }
    }
};
}
