#pragma once

#include <span>

#include <Usagi/Library/Noncopyable.hpp>
#include <Usagi/Module/Common/Math/Matrix.hpp>

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

    struct ImageSyncObjects
    {
        VulkanUniqueSemaphore sem_image_available;
        VulkanUniqueSemaphore sem_render_finished;
        VulkanUniqueFence fence_render_finished;

        ImageSyncObjects(
            VulkanUniqueSemaphore sem_image_available,
            VulkanUniqueSemaphore sem_render_finished,
            VulkanUniqueFence fence_render_finished)
            : sem_image_available(std::move(sem_image_available))
            , sem_render_finished(std::move(sem_render_finished))
            , fence_render_finished(std::move(fence_render_finished))
        {
        }
    };
    std::vector<ImageSyncObjects> mSyncObjectPool;

    void insert_new_sync_objects();

    ImageSyncObjects & locate_available_sync_objects();

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
    struct NextImage
    {
        vk::Image image;
        // the rendering commands should wait on this semaphore for the
        // image to be available
        vk::Semaphore sem_image_available;
        // signal this semaphore in rendering commands and pass it to
        // `present()`
        // todo: this should not be managed by the swapchain
        vk::Semaphore sem_render_finished;
        // pass the message back here so we know that this set of sync
        // objects can be reused.
        vk::Fence fence_render_finished;

        std::uint32_t priv_image_index = -1;
    };

    NextImage acquire_next_image();

    void present(
        const NextImage &image,
        std::span<vk::Semaphore> wait_semaphores);
};
}
