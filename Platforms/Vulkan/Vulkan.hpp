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
#define USAGI_DEF_VULKAN_HANDLE(type, vk_type) \
using VulkanUnique##type = \
    vk::UniqueHandle<vk::vk_type, vk::DispatchLoaderDynamic> \
/**/

USAGI_DEF_VULKAN_HANDLE(Instance, Instance);
USAGI_DEF_VULKAN_HANDLE(DebugMessenger, DebugUtilsMessengerEXT);
USAGI_DEF_VULKAN_HANDLE(Device, Device);
USAGI_DEF_VULKAN_HANDLE(Semaphore, Semaphore);
USAGI_DEF_VULKAN_HANDLE(Fence, Fence);
USAGI_DEF_VULKAN_HANDLE(CommandPool, CommandPool);
USAGI_DEF_VULKAN_HANDLE(CommandBuffer, CommandBuffer);
USAGI_DEF_VULKAN_HANDLE(Surface, SurfaceKHR);
USAGI_DEF_VULKAN_HANDLE(Swapchain, SwapchainKHR);
USAGI_DEF_VULKAN_HANDLE(ShaderModule, ShaderModule);
USAGI_DEF_VULKAN_HANDLE(Pipeline, Pipeline);
USAGI_DEF_VULKAN_HANDLE(RenderPass, RenderPass);
USAGI_DEF_VULKAN_HANDLE(DescriptorSetLayout, DescriptorSetLayout);
USAGI_DEF_VULKAN_HANDLE(PipelineLayout, PipelineLayout);
USAGI_DEF_VULKAN_HANDLE(DeviceMemory, DeviceMemory);

#undef USAGI_DEF_VULKAN_HANDLE
}
