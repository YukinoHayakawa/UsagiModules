#include "RbVulkanSwapchain.hpp"

#include <Usagi/Modules/Resources/ResWindowManager/RbNativeWindow.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

namespace usagi
{
ResourceState RbVulkanSwapchain::construct(
    ResourceConstructDelegate<RbVulkanSwapchain> &delegate)
{
    // Get native window
    const auto window = 
        delegate.resource_apply<RbNativeWindow>(arguments()).await();

    auto &swapchain = delegate.allocate(window.get());
    swapchain.create(
        window->surface_size().cast<std::uint32_t>(),
        vk::Format::eB8G8R8A8Unorm
    );

    return ResourceState::READY;
}
}
