#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapVulkanObjectManager.hpp"

namespace usagi
{
/*
 * Provide transient command list to Render Systems. HeapManager only tracks
 * the references of the produced resources. Resource identified by a
 * (FrameId, SystemType, ThreadId) tuple.
 */
class RbVulkanCommandListGraphics
{
    // std::size_t mFrameNumber = -1;
    // std::type_index mSystemType;
    std::thread::id mThreadId;

public:
    RbVulkanCommandListGraphics(
        std::size_t frame_number,
        std::type_index system_type,
        std::thread::id thread_id);

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanCommandListGraphics;

    using Transient = void;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanCommandListGraphics> &delegate);
};
}
