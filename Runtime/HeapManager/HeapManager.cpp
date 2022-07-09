#include "HeapManager.hpp"

#include <Usagi/Runtime/Task/TaskExecutor.hpp>
#include <Usagi/Modules/Runtime/Executive/TaskExecutorSynchronized.hpp>

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
    task->set_executor(&executor);
    executor.submit(std::move(task), { });
}

void RequestContextDeleter::operator()(
    const ResourceBuildContextCommon *context) const
{
    context->manager->deallocate_request_context(*context);
}
}

namespace usagi
{
bool HeapManager::ResourceEntryComparator::operator()(
    const std::unique_ptr<ResourceEntryBase> &lhs,
    const std::unique_ptr<ResourceEntryBase> &rhs) const
{
    return lhs->descriptor < rhs->descriptor;
}

bool HeapManager::ResourceEntryComparator::operator()(
    const HeapResourceDescriptor &lhs,
    const std::unique_ptr<ResourceEntryBase> &rhs) const
{
    return lhs < rhs->descriptor;
}

bool HeapManager::ResourceEntryComparator::operator()(
    const std::unique_ptr<ResourceEntryBase> &lhs,
    const HeapResourceDescriptor &rhs) const
{
    return lhs->descriptor < rhs;
}

HeapResourceDescriptor HeapManager::make_unique_descriptor()
{
    return { DummyBuilderId, mUniqueResourceIdCounter++ };
}

void HeapManager::deallocate_request_context(
    const ResourceBuildContextCommon &context)
{
    mRequestContextPool.deallocate(context.context_index);
}
}
