#include "RbVulkanCommandListGraphics.hpp"

namespace usagi
{
RbVulkanCommandListGraphics::RbVulkanCommandListGraphics(
    const std::size_t frame_number,
    std::type_index system_type,
    const std::thread::id thread_id)
    // : mFrameNumber(frame_number)
    // , mSystemType(std::move(system_type))
    : mThreadId(thread_id)
{
}

ResourceState RbVulkanCommandListGraphics::construct(
    ResourceConstructDelegate<RbVulkanCommandListGraphics> &delegate)
{
    // Communicate with HeapVulkanObjectManager to get the command pool for
    // the current thread.
    delegate.allocate(mThreadId);

    // Allocate a command buffer for the requesting system.

    return ResourceState::READY;
}
}
