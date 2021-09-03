#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#pragma push_macro("_WINDOWS_")
    // define the minimal required types used by vulkan, unless windows.h
    // is already included.
    #ifndef _WINDOWS_
        #define _WINDOWS_
        struct HINSTANCE__ { int unused; }; typedef struct HINSTANCE__* HINSTANCE;
        struct HWND__ { int unused; }; typedef struct HWND__* HWND;
        struct HMONITOR__ { int unused; }; typedef struct HMONITOR__* HMONITOR;
        typedef void *HANDLE;
        typedef unsigned long DWORD;
        typedef void *LPVOID;
        typedef int BOOL;
        typedef const wchar_t* LPCWSTR;
        typedef struct _SECURITY_ATTRIBUTES {
            DWORD nLength;
            LPVOID lpSecurityDescriptor;
            BOOL bInheritHandle;
        } SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
    #endif
#endif

// do not use static dispatcher
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

#ifdef _WIN32
#pragma pop_macro("_WINDOWS_")
#endif

namespace usagi
{
using VulkanUniqueInstance =
    vk::UniqueHandle<vk::Instance, vk::DispatchLoaderDynamic>;
using VulkanUniqueDebugMessenger =
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>;
using VulkanUniqueDevice =
    vk::UniqueHandle<vk::Device, vk::DispatchLoaderDynamic>;
using VulkanUniqueSemaphore =
    vk::UniqueHandle<vk::Semaphore, vk::DispatchLoaderDynamic>;
using VulkanUniqueFence =
    vk::UniqueHandle<vk::Fence, vk::DispatchLoaderDynamic>;
using VulkanUniqueCommandPool =
    vk::UniqueHandle<vk::CommandPool, vk::DispatchLoaderDynamic>;
using VulkanUniqueCommandBuffer =
    vk::UniqueHandle<vk::CommandBuffer, vk::DispatchLoaderDynamic>;
using VulkanUniqueSurface =
    vk::UniqueHandle<vk::SurfaceKHR, vk::DispatchLoaderDynamic>;
using VulkanUniqueSwapchain =
    vk::UniqueHandle<vk::SwapchainKHR, vk::DispatchLoaderDynamic>;
}
