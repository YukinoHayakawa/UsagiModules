#include "RbStandardTaskGraph.hpp"

#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

#include "Streams.hpp"

namespace usagi
{
ResourceState RbStandardTaskGraph::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path,
    const std::size_t max_tasks)
{
    const auto asset = delegate.resource<RbAssetMemoryView>(path).await();
    auto in = stream_from_string_view(asset->to_string_view());

    std::size_t num_tasks = 0;
    in >> num_tasks;

    // reserve space for dummy start/end tasks
    num_tasks += 2;

    // limit number of tasks when required
    if(max_tasks != 0)
        num_tasks = std::min(num_tasks, max_tasks);

    fmt::print("Number of tasks: {}\n", num_tasks);

    TaskGraph tg { num_tasks };

    int task_id = 0, proc_time = 0, num_predecessors = 0, predecessor_id = 0;
    for(int i = 0; i < num_tasks; ++i)
    {
        in >> task_id >> proc_time >> num_predecessors;
        assert(task_id == i);
        assert(num_predecessors <= num_tasks);
        tg.vertex(task_id).base_comp_cost = proc_time;
        for(int j = 0; j < num_predecessors; ++j)
        {
            in >> predecessor_id;
            assert(predecessor_id < num_tasks);
            tg.add_edge(predecessor_id, task_id, 0);
        }
    }

    delegate.emplace(std::move(tg));

    return ResourceState::READY;
}
}
