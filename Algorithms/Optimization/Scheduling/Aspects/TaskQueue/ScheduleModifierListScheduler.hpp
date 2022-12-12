#pragma once

#include <type_traits>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecuteTask.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleNodeProcessorReady.hpp>

#include "ScheduleMixinTaskQueue.hpp"

namespace usagi
{
template <typename ProcAllocPolicy>
struct ScheduleModifierListScheduler
{
    template <typename Graph>
    void operator()(Graph &&graph) requires
        std::is_base_of_v<
            ScheduleMixinTaskQueue<
                std::decay_t<Graph>,
                typename std::decay_t<Graph>::TaskIndexT,
                typename std::decay_t<Graph>::VertexIndexT
            >,
            std::decay_t<Graph>
        >
    {
        using TaskIndexT = typename std::decay_t<Graph>::TaskIndexT;

        while(!graph.task_queue.empty())
        {
            auto task_vertex_id = graph.task_queue.top();
            graph.task_queue.pop();

            auto [proc_idx, proc_ref] = 
                ProcAllocPolicy()(std::forward<Graph>(graph));

            static_assert(std::is_reference_v<decltype(proc_ref)>);

            graph.template add_edge<
                ScheduleNodeProcessorReady,
                ScheduleNodeExecuteTask<TaskIndexT>
            >(proc_idx, task_vertex_id);
        }
    }
};
}
