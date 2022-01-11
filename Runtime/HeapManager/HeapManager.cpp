#include "HeapManager.hpp"

#include <Usagi/Runtime/TaskExecutor.hpp>

namespace usagi
{
void HeapManager::submit_build_task(
    TaskExecutor *executor,
    std::unique_ptr<Task> task)
{
    assert(executor);
    executor->submit(std::move(task));
}
}
