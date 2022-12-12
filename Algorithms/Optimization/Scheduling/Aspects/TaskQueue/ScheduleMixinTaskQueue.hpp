#pragma once

#include <queue>

namespace usagi
{
template <
    typename Graph,
    typename TaskIndexT,
    typename VertexIndexT
>
struct ScheduleMixinTaskQueue
{
    struct TaskCompareFunc
    {
        Graph *graph;

        // the task indices might be mapped/encoded
        bool operator()(const VertexIndexT lhs, const VertexIndexT rhs)
        {
            using Exec = ScheduleNodeExecuteTask<TaskIndexT>;

            auto &l = graph->template vertex<Exec>(lhs);
            auto &r = graph->template vertex<Exec>(rhs);

            return graph->task_priority(l.task_id) < 
                graph->task_priority(r.task_id);
        }
    };

    // store the vertex indices of tasks to be scheduled
    std::priority_queue<
        VertexIndexT,
        std::vector<VertexIndexT>,
        TaskCompareFunc
    > task_queue;

    ScheduleMixinTaskQueue()
        : task_queue(TaskCompareFunc { .graph = static_cast<Graph *>(this) })
    {
    }

    void enqueue_task(const VertexIndexT task_vertex_index)
    {
        task_queue.push(task_vertex_index);
    }
};
}
