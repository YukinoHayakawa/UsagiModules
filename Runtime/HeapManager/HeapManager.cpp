#include "HeapManager.hpp"

#include <Usagi/Runtime/Task/TaskExecutor.hpp>
#include <Usagi/Modules/Runtime/Executive/TaskExecutorSynchronized.hpp>

namespace usagi
{
void HeapManager::submit_build_task(
    TaskExecutor *executor,
    std::unique_ptr<Task> task)
{
    assert(executor);
    executor->submit(std::move(task));
}

void HeapManager::run_build_task_synced(std::unique_ptr<Task> task)
{
    TaskExecutorSynchronized executor;
    executor.submit(std::move(task), {});
}
}
