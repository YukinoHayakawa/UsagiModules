#pragma once

#include <span>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Modules/Common/Math/Matrix.hpp>

#include "Vulkan.hpp"

namespace usagi
{
class VulkanGpuDevice;

class VulkanSwapchain : Noncopyable
{
    VulkanGpuDevice *mDevice = nullptr;

    VulkanUniqueSurface mSurface;
    vk::SurfaceFormatKHR mFormat;
    Vector2u32 mSize;

    VulkanUniqueSwapchain mSwapchain;
    std::vector<vk::Image> mImages;
    vk::Result mLastResult = vk::Result::eSuccess;

    static vk::SurfaceFormatKHR select_surface_format(
        const std::vector<vk::SurfaceFormatKHR> &surface_formats,
        vk::Format preferred_image_format);
    vk::Extent2D select_surface_extent(
        const Vector2u32 &size,
        const vk::SurfaceCapabilitiesKHR &surface_capabilities) const;
    static vk::PresentModeKHR select_present_mode(
        const std::vector<vk::PresentModeKHR> &present_modes);
    std::uint32_t select_presentation_queue_family() const;

    void get_swapchain_images();
    void skip_image(vk::Semaphore semaphore);
    void recreate();

public:
    VulkanSwapchain(
        VulkanGpuDevice *device,
        VulkanUniqueSurface vk_surface_khr);

    void create(const Vector2u32& size, vk::Format format);
    void resize(const Vector2u32& size);

    // GpuBufferFormat format() const override;
    Vector2u32 size() const { return mSize; }

    // the exact structure and internal member types of this struct is
    // supposed to be transparent to the user.
    struct ImageInfo
    {
        vk::Image image;

    private:
        friend class VulkanSwapchain;

        std::uint32_t mImageIndex = -1;
    };

    ImageInfo acquire_next_image(vk::Semaphore signal_sem_image_avail);

    void present(
        const ImageInfo &image,
        std::span<vk::Semaphore> wait_semaphores);
};
}
