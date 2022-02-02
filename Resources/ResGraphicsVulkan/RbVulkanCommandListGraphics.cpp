#include "RbVulkanCommandListGraphics.hpp"

namespace usagi
{
ResourceState RbVulkanCommandListGraphics::construct(
    ResourceConstructDelegate<RbVulkanCommandListGraphics> &delegate)
{
    // Communicate with HeapVulkanObjectManager to get the command pool for
    // the current thread.

    // Allocate a command buffer for the requesting system.

    return ResourceState::READY;
}
}
