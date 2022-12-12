#pragma once

#include <Usagi/Modules/Resources/ResTaskGraphs/TaskGraph.hpp>

namespace usagi
{
struct ScheduleMixinTaskGraph
{
    using TaskIndexT = TaskGraph::VertexIndexT;

    TaskGraph task_graph;

    TaskIndexT num_tasks() const
    {
        return task_graph.num_vertices();
    }

    auto descendant_tasks(const TaskIndexT task) const
    {
        return task_graph.adjacent_vertices(task);
    }
};
}
