﻿#include "VulkanSwapchain.hpp"

#include <ranges>

#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "VulkanEnum.hpp"
#include "VulkanGpuDevice.hpp"

namespace usagi
{
VulkanSwapchain::VulkanSwapchain(VulkanUniqueSurface vk_surface_khr)
    : mSurface { std::move(vk_surface_khr) }
    // todo linear color format? https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
    , mFormat { vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear }
{
}

// https://stackoverflow.com/a/59830015
void VulkanSwapchain::skip_image(vk::Semaphore semaphore)
{
    vk::SubmitInfo2KHR info;
    info.waitSemaphoreInfoCount = 1;
    vk::SemaphoreSubmitInfoKHR sem_info;
    sem_info.semaphore = semaphore;
    sem_info.deviceIndex = 0;
    sem_info.stageMask = Vulkan_GpuPipelineStage(
        GpuPipelineStage::ALL_COMMANDS
    );
    info.pWaitSemaphoreInfos = &sem_info;
    graphics_queue().submit2KHR(
        std::array { info },
        nullptr,
        dispatch()
    );
}

void VulkanSwapchain::recreate()
{
    create(mSize, mFormat.format);
}

VulkanSwapchain::ImageInfo VulkanSwapchain::acquire_next_image(
    vk::Semaphore signal_sem_image_avail)
{
    assert(mSwapchain);

    if(mLastResult != vk::Result::eSuccess)
        recreate();

    ImageInfo image;

    // bug sometimes hangs when debugging with RenderDoc
    const auto result = device()->device().acquireNextImageKHR(
        mSwapchain.get(),
        UINT64_MAX, // 1000000000u, // 1s
        signal_sem_image_avail,
        nullptr,
        &image.mImageIndex,
        dispatch()
    );
    switch(result)
    {
        case vk::Result::eSuboptimalKHR:
            LOG(warn, "Suboptimal swapchain");
            skip_image(signal_sem_image_avail);
            recreate();
            return acquire_next_image(signal_sem_image_avail);
        case vk::Result::eSuccess: break;
        case vk::Result::eNotReady:
            USAGI_THROW(std::runtime_error(
                "Currently no usable swapchain image."));
        case vk::Result::eTimeout:
            USAGI_THROW(std::runtime_error(
                "acquireNextImageKHR() timed out."));

        case vk::Result::eErrorOutOfDateKHR:
            LOG(info, "Swapchain is out-of-date, recreating");
            create(mSize, mFormat.format);
            return acquire_next_image(signal_sem_image_avail);

        default: USAGI_THROW(std::runtime_error(
            "acquireNextImageKHR() failed."));
    }

    image.image = mImages[image.mImageIndex];

    return image;
}

void VulkanSwapchain::present(
    const ImageInfo &image,
    const std::span<vk::Semaphore> wait_semaphores)
{
    vk::PresentInfoKHR info;

    info.setWaitSemaphoreCount(static_cast<uint32_t>(wait_semaphores.size()));
    info.setPWaitSemaphores(wait_semaphores.data());
    const auto sc_handle = mSwapchain.get();
    info.setSwapchainCount(1);
    info.setPSwapchains(&sc_handle);
    info.setPImageIndices(&image.mImageIndex);

    // https://github.com/KhronosGroup/Vulkan-Docs/issues/595
    // Seems that after the semaphores waited on have all signaled,
    // the presentation will be finished almost immediately, so
    // tracking the reference of the semaphores is not necessary (if it is
    // possible).

    // If the swapchain is suboptimal or out-of-date, it will be recreated
    // during next call of acquireNextImage().
    switch(mLastResult = present_queue().presentKHR(
        &info,
        dispatch()))
    {
        case vk::Result::eSuccess: break;
        case vk::Result::eSuboptimalKHR:
            LOG(warn, "Presented to a suboptimal swapchain.");
            break;
        case vk::Result::eErrorOutOfDateKHR:
            LOG(error, "Presented to a out-of-date swapchain.");
            break;
        default: ;
    }
}

// todo: allow choosing colorspace
vk::SurfaceFormatKHR VulkanSwapchain::select_surface_format(
    const std::vector<vk::SurfaceFormatKHR> &surface_formats,
    const vk::Format preferred_image_format)
{
    // If the list contains only one entry with undefined format
    // it means that there are no preferred surface formats and any can be
    // chosen
    if(surface_formats.size() == 1 &&
        surface_formats[0].format == vk::Format::eUndefined)
        return {
            preferred_image_format,
            vk::ColorSpaceKHR::eSrgbNonlinear
        };

    // todo: fallback
    for(auto &&surface_format : surface_formats)
    {
        if(surface_format.format == preferred_image_format)
        {
            return surface_format;
        }
    }

    // bug: NVIDIA card has only limited color formats
    // Return the first format from the list
    return surface_formats[0];
}

vk::Extent2D VulkanSwapchain::select_surface_extent(
    const Vector2u32 &size,
    const vk::SurfaceCapabilitiesKHR &surface_capabilities) const
{
    // currentExtent is the current width and height of the surface, or
    // the special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that the
    // surface size will be determined by the extent of a swapchain
    // targeting the surface.
    if(surface_capabilities.currentExtent.width == 0xFFFFFFFF)
    {
        vk::Extent2D swap_chain_extent = { size.x(), size.y() };
        swap_chain_extent.width = std::clamp(swap_chain_extent.width,
            surface_capabilities.minImageExtent.width,
            surface_capabilities.maxImageExtent.width
        );
        swap_chain_extent.height = std::clamp(swap_chain_extent.height,
            surface_capabilities.minImageExtent.height,
            surface_capabilities.maxImageExtent.height
        );
        return swap_chain_extent;
    }
    // Most of the cases we define size of the swap_chain images equal to
    // current window's size
    return surface_capabilities.currentExtent;
}

vk::PresentModeKHR VulkanSwapchain::select_present_mode(
    const std::vector<vk::PresentModeKHR> &present_modes)
{
    // todo: allow disable v-sync
    // prefer mailbox mode to achieve triple buffering
    if(std::ranges::find(present_modes, vk::PresentModeKHR::eMailbox)
        != present_modes.end())
        return vk::PresentModeKHR::eMailbox;
    return vk::PresentModeKHR::eFifo;
}

std::uint32_t VulkanSwapchain::select_presentation_queue_family() const
{
    // todo use vkGetPhysicalDeviceWin32PresentationSupportKHR
    const auto queue_families =
        device()->physical_device().getQueueFamilyProperties(
            dispatch()
        );
    for(auto i = queue_families.begin(); i != queue_families.end(); ++i)
    {
        const auto queue_index =
            static_cast<uint32_t>(i - queue_families.begin());
        if(device()->physical_device().getSurfaceSupportKHR(
            queue_index,
            mSurface.get(),
            dispatch()
        )) return queue_index;
    }
    USAGI_THROW(std::runtime_error(
        "No queue family supporting WSI was found."));
}

void VulkanSwapchain::resize(const Vector2u32 &size)
{
    if(mSize != size)
        create(size, mFormat.format);
}

void VulkanSwapchain::create(
    const Vector2u32 &size,
    vk::Format image_format)
{
    assert(size.x() > 0 && size.y() > 0);

    // Ensure that no operation involving the swapchain images is outstanding.
    // Since acquireNextImage() and drawing operations aren't parallel,
    // as long as the device is idle, it won't happen.
    device()->device().waitIdle(dispatch());

    LOG(info, "Creating swapchain");

    // todo: query using vkGetPhysicalDeviceWin32PresentationSupportKHR
    // mPresentationQueueFamilyIndex = selectPresentationQueueFamily();
    // LOG(info, "Using queue family {} for presentation",
    //     mPresentationQueueFamilyIndex);

    const auto surface_capabilities =
        device()->physical_device().getSurfaceCapabilitiesKHR(
            mSurface.get(), dispatch());
    const auto surface_formats =
        device()->physical_device().getSurfaceFormatsKHR(
            mSurface.get(), dispatch());
    const auto surface_present_modes =
        device()->physical_device().getSurfacePresentModesKHR(
            mSurface.get(), dispatch());

    vk::SwapchainCreateInfoKHR create_info;

    create_info.setSurface(mSurface.get());
    const auto vk_format = select_surface_format(surface_formats, image_format);
    LOG(info, "Surface format: {}", to_string(vk_format.format));
    LOG(info, "Surface colorspace: {}", to_string(vk_format.colorSpace));
    create_info.setImageFormat(vk_format.format);
    create_info.setImageColorSpace(vk_format.colorSpace);
    create_info.setImageExtent(
        select_surface_extent(size, surface_capabilities)
    );
    LOG(info,
        "Surface extent: {}x{}",
        create_info.imageExtent.width,
        create_info.imageExtent.height
    );

    create_info.setPresentMode(select_present_mode(surface_present_modes));
    // todo: mailbox not available on my R9 290X
    // todo triple buffering
    if(create_info.presentMode == vk::PresentModeKHR::eMailbox)
        // Ensures non-blocking vkAcquireNextImageKHR() in mailbox mode.
        // See 3.6.12 of http://vulkan-spec-chunked.ahcox.com/apes03.html
        create_info.setMinImageCount(surface_capabilities.minImageCount + 1);
    else
        create_info.setMinImageCount(surface_capabilities.minImageCount);

    create_info.setImageArrayLayers(1);
    create_info.setImageSharingMode(vk::SharingMode::eExclusive);
    create_info.setPreTransform(surface_capabilities.currentTransform);

    create_info.setImageUsage(
        // For direct rendering
        vk::ImageUsageFlagBits::eColorAttachment |
        // For vkCmdClearColorImage
        vk::ImageUsageFlagBits::eTransferDst
    );

    create_info.setOldSwapchain(mSwapchain.get());

    mSwapchain = VulkanDeviceAccess::create(create_info);
    mFormat = vk_format;
    mSize = { create_info.imageExtent.width, create_info.imageExtent.height };

    get_swapchain_images();
}

void VulkanSwapchain::get_swapchain_images()
{
    mImages = device()->device().getSwapchainImagesKHR(
        mSwapchain.get(),
        dispatch()
    );
}
}
