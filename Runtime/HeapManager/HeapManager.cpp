#include "HeapManager.hpp"

namespace usagi::details::heap_manager
{
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
