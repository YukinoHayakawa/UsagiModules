#pragma once

#include <map>
#include <thread>

#include <Usagi/Modules/Runtime/HeapManager/Heap.hpp>
#include <Usagi/Modules/Runtime/HeapManager/details/HeapResourceDescriptor.hpp>

#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipelineCompiler.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>

namespace usagi
{
class HeapVulkanObjectManager : VulkanDeviceAccess, public Heap
{

    std::map<HeapResourceIdT, VulkanShaderModule> mShaderModules;
    std::map<HeapResourceIdT, VulkanGraphicsPipeline> mGraphicsPipelines;
    // std::map<HeapResourceIdT, VulkanGraphicsPipeline> mGraphicsPipelines;
    // // todo: how to know which threads are dead? make threads into resources?
    // std::map<std::thread::id, VulkanCommandPool> mCommandPools;

public:
    explicit HeapVulkanObjectManager(VulkanGpuDevice *device)
        : VulkanDeviceAccess(device)
    {
    }

    template <typename Object>
    const auto & resource(const HeapResourceIdT id)
    {
        auto op = [&](auto &manager) -> auto &
        {
            const auto it = manager.find(id);
            assert(it != manager.end());
            return it->second;
        };
        if constexpr(std::same_as<Object, VulkanShaderModule>)
            return op(mShaderModules);
        else if constexpr(std::same_as<Object, VulkanGraphicsPipeline>)
            return op(mGraphicsPipelines);
    }

    template <typename Object, typename CreateInfo, typename... Args>
    const auto & allocate(
        const HeapResourceIdT id,
        CreateInfo &&info,
        Args &&...args)
    {
        auto op = [&](auto &manager, auto create_func) -> auto &
        {
            // static_assert(std::same_as<CreateInfo, vk::ShaderModuleCreateInfo>);

            auto [it, inserted] = manager.try_emplace(
                id,
                create_func(info),
                std::forward<Args>(args)...
            );
            assert(inserted);
            return it->second;
        };
        if constexpr(std::same_as<Object, VulkanShaderModule>)
        {
            static_assert(std::same_as<
                std::remove_cvref_t<CreateInfo>,
                vk::ShaderModuleCreateInfo
            >);
            return op(mShaderModules, [&](auto &&i) {
                return create_shader_module(i);
            });
        }
        else if constexpr(std::same_as<Object, VulkanGraphicsPipeline>)
        {
            static_assert(std::same_as<
                std::remove_cvref_t<CreateInfo>,
                VulkanGraphicsPipelineCompiler
            >);
            // bug this is a temp hack
            VulkanGraphicsPipelineCompiler &compiler = info;
            // inject device access
            static_cast<VulkanDeviceAccess&>(compiler) =
                static_cast<VulkanDeviceAccess&>(*this);
            return op(mGraphicsPipelines, [&](auto &&i) {
                return i.compile();
            });
        }
    }
};
}
