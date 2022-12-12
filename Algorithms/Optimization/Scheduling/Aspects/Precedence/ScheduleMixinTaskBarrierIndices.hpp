#pragma once

#include <map>

namespace usagi
{
template <
    typename TaskIndexT,
    typename VertexIndexT
>
struct ScheduleMixinTaskBarrierIndices
{
    std::map<TaskIndexT, VertexIndexT> task_begin_barrier_indices;
    std::map<TaskIndexT, VertexIndexT> task_end_barrier_indices;

    auto task_begin_barrier_index(TaskIndexT task) const
    {
        return task_begin_barrier_indices.at(task);
    }

    auto task_end_barrier_index(TaskIndexT task) const
    {
        return task_end_barrier_indices.at(task);
    }
};
}
