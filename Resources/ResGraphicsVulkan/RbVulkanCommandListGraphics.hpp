#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
class RbVulkanCommandListGraphics
{
    std::size_t mCurrentFrameNumber;

public:
    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanCommandListGraphics;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanCommandListGraphics> &delegate);
};
}
