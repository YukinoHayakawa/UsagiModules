#include "HeapManager.hpp"

#include <Usagi/Runtime/Task/TaskExecutor.hpp>
#include <Usagi/Modules/Runtime/Executive/TaskExecutorSynchronized.hpp>

// Included to allow the dtor of HeapManager can deconstruct the heap objects.
#include "Heap.hpp"

namespace usagi::details::heap_manager
{
void submit_build_task(
    TaskExecutor *executor,
    std::unique_ptr<Task> task)
{
    assert(executor);
    executor->submit(std::move(task));
}

void run_build_task_synced(std::unique_ptr<ResourceBuildTaskBase> task)
{
    TaskExecutorSynchronized executor;
    // todo refactor
    task->mExecutor = &executor;
    executor.submit(std::move(task), { });
}
}

namespace usagi
{
HeapManager::~HeapManager()
{
}
}
