#pragma once

#include <concepts>

namespace usagi
{
template <typename Graph>
concept SubtaskEnabledSchedule = 
    requires(
        Graph g,
        typename Graph::TaskIndexT task_index)
{
    typename Graph::TaskIndexT;

    { g.num_subtasks(task_index) } ->
        std::convertible_to<typename Graph::TaskIndexT>;
    { g.composed_task_id(task_index, task_index) } ->
        std::convertible_to<typename Graph::TaskIndexT>;
    { g.subtask_exec_time(task_index, task_index) } ->
        std::convertible_to<float>;
};
}
