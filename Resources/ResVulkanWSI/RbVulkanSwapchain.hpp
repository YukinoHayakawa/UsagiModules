#pragma once

#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/IO/Windowing/NativeWindowState.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanSwapchain.hpp>
#include <Usagi/Modules/Resources/ResGraphicsVulkan/HeapVulkanObjectManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/Runtime/HeapManager/TransparentArg.hpp>

namespace usagi
{
class RbVulkanSwapchain
    : ArgumentStorage< // todo don't require params of the native window
        std::uint64_t,                      // identifier
        TransparentArg<std::string_view>,   // default title
        TransparentArg<Vector2f>,           // default position
        TransparentArg<Vector2f>,           // default size
        TransparentArg<float>,              // default dpi scaling
        TransparentArg<NativeWindowState>   // default state   // default state
    >
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapVulkanObjectManager;
    using ProductT = VulkanSwapchain;

    ResourceState construct(
        ResourceConstructDelegate<RbVulkanSwapchain> &delegate);
};
}
