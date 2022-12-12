#include "ScheduleHomogeneousSplittable.hpp"

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Precedence/ScheduleModifierInsertPrecedenceConstraints.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleModifierCreateProcessors.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleModifierInsertRoot.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Subtasks/ScheduleModifierInsertSubtasks.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/TaskQueue/ScheduleModifierListScheduler.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/TaskQueue/ScheduleModifierStartScheduling.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/ProcessorAllocation/ProcessorAllocationEarliestAvailable.hpp>

namespace usagi
{
constexpr auto max_subtasks = 2;
constexpr auto num_proc = 4;

ScheduleMixinTaskGraph::TaskIndexT
ScheduleHomogeneousSplittable::num_subtasks(TaskIndexT task_index) const
{
    return max_subtasks;
}

ScheduleMixinTaskGraph::TaskIndexT
ScheduleHomogeneousSplittable::composed_task_id(
    TaskIndexT task_idx,
    TaskIndexT subtask_idx) const
{
    return task_idx * max_subtasks + subtask_idx;
}

float ScheduleHomogeneousSplittable::subtask_exec_time(
    TaskIndexT task_idx,
    TaskIndexT subtask_idx) const
{
    return 12;
}

void ScheduleHomogeneousSplittable::do_schedule()
{
    // create the task graph

    task_graph.resize(7);
    task_graph.add_edge(0, 1);
    task_graph.add_edge(1, 2);
    task_graph.add_edge(1, 3);
    task_graph.add_edge(2, 4);
    task_graph.add_edge(4, 5);
    task_graph.add_edge(3, 5);
    task_graph.add_edge(5, 6);

    // create schedule root node
    ScheduleModifierInsertRoot()(*this);

    // init processors
    ScheduleModifierCreateProcessors()(*this, num_proc);

    // create task begin & finish barriers & precedence constraints 
    ScheduleModifierInsertPrecedenceConstraints()(*this);

    // create task execution nodes
    ScheduleModifierInsertSubtasks()(*this);

    // push the source task
    ScheduleModifierStartScheduling()(*this);

    // create the schedule
    ScheduleModifierListScheduler<ProcessorAllocationEarliestAvailable>()(*this);
}
}
