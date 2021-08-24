#pragma once

#include <span>

#include <vulkan/vulkan.hpp>

#include <Usagi/Library/Noncopyable.hpp>
#include <Usagi/Module/Common/Math/Matrix.hpp>

namespace usagi
{
class VulkanGpuDevice;

// todo
class VulkanSwapchain : Noncopyable
{
    VulkanGpuDevice *mDevice;

    vk::UniqueSurfaceKHR mSurface;
    vk::SurfaceFormatKHR mFormat;
    Vector2u32 mSize;

    vk::UniqueSwapchainKHR mSwapchain;

    static inline constexpr uint32_t INVALID_IMAGE_INDEX = -1;
    uint32_t mCurrentImageIndex = INVALID_IMAGE_INDEX;
    std::vector<vk::Image> mImages;
    int mImagesInUse = 0;

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

    void create(const Vector2u32 &size, vk::Format format);
    void resize(const Vector2u32 &size);

public:
    VulkanSwapchain(
        VulkanGpuDevice *device,
        vk::UniqueSurfaceKHR vk_surface_khr);

    // GpuBufferFormat format() const override;
    Vector2u32 size() const { return mSize; }

    vk::Semaphore acquire_next_image();
    vk::Image current_image();

    void present(std::span<vk::Semaphore> wait_semaphores);
};
}
