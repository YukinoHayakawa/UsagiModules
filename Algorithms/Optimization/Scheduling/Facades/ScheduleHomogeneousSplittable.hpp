#pragma once

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/ScheduleConstraintGraph.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Events/ScheduleEventHandlerFallbackNoop.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecuteTask.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecutionBarrier.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Precedence/ScheduleEventHandlerPrecedenceConstraints.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Precedence/ScheduleMixinTaskBarrierIndices.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleMixinNodeVisitorAvailableProcessors.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleNodeProcessorReady.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleMixinRootNodeIndex.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleNodeRoot.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/TaskGraph/ScheduleMixinTaskGraph.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/TaskQueue/ScheduleMixinTaskQueue.hpp>

namespace usagi
{
using ScheduleVertexT = std::uint32_t;

struct ScheduleHomogeneousSplittable
    : ScheduleMixinTaskGraph
    , ScheduleMixinRootNodeIndex<ScheduleVertexT>
    , ScheduleMixinTaskBarrierIndices<
        ScheduleMixinTaskGraph::TaskIndexT,
        ScheduleVertexT
    >
    , ScheduleMixinTaskQueue<
        ScheduleHomogeneousSplittable,
        ScheduleMixinTaskGraph::TaskIndexT,
        ScheduleVertexT
    >
    , ScheduleMixinNodeVisitorAvailableProcessors<
        ScheduleHomogeneousSplittable
    >
    , ScheduleEventHandlerPrecedenceConstraints<
        ScheduleHomogeneousSplittable,
        ScheduleVertexT,
        ScheduleMixinTaskGraph::TaskIndexT
    >
    , ScheduleEventHandlerFallbackNoop<
        ScheduleVertexT
    >
    , ScheduleConstraintGraph<
        ScheduleVertexT,
        ScheduleHomogeneousSplittable,
        ScheduleNodeRoot,
        ScheduleNodeProcessorReady,
        ScheduleNodeExecuteTask<ScheduleMixinTaskGraph::TaskIndexT>,
        ScheduleNodeExecutionBarrier<ScheduleMixinTaskGraph::TaskIndexT>
    >
{
    using ScheduleEventHandlerFallbackNoop::on_edge_added;
    using ScheduleEventHandlerFallbackNoop::on_vertex_added;
    using ScheduleEventHandlerPrecedenceConstraints::on_edge_added;

    using VertexIndexT = ScheduleVertexT;

    TaskIndexT num_subtasks(TaskIndexT task_index) const;
    TaskIndexT composed_task_id(
        TaskIndexT task_idx,
        TaskIndexT subtask_idx) const;
    float subtask_exec_time(
        TaskIndexT task_idx,
        TaskIndexT subtask_idx) const;

    int task_priority(TaskIndexT task_idx)
    {
        return 1;
    }

    void do_schedule();
};
}
