#pragma once

#include <type_traits>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecuteTask.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecutionBarrier.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Precedence/ScheduleMixinTaskBarrierIndices.hpp>

#include "SubtaskEnabledSchedule.hpp"

namespace usagi
{
// insert subtasks between task begin & end barriers and link them together.
struct ScheduleModifierInsertSubtasks
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
        // num_subtasks & composed_task_id & subtask_exec_time
        > && SubtaskEnabledSchedule<std::decay_t<Graph>>
    {
        using TaskIndexT = typename std::decay_t<Graph>::TaskIndexT;
        using Barrier = ScheduleNodeExecutionBarrier<TaskIndexT>;
        using Exec = ScheduleNodeExecuteTask<TaskIndexT>;

        for(TaskIndexT i = 0; i < graph.num_tasks(); ++i)
        {
            for(TaskIndexT j = 0; j < graph.num_subtasks(i); ++j)
            {
                auto [sub_idx, sub_ref] = graph.template add_vertex<Exec>();
                sub_ref.task_id = graph.composed_task_id(i, j);
                sub_ref.exec_time = graph.subtask_exec_time(i, j);

                graph.template add_edge<Barrier, Exec>(
                    graph.task_begin_barrier_index(i),
                    sub_idx
                );
                graph.template add_edge<Exec, Barrier>(
                    sub_idx, 
                    graph.task_end_barrier_index(i)
                );
            }
        }
    }
};
}
