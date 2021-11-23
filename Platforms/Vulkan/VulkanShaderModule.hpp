#pragma once

#include <spirv_cross/spirv_cross.hpp>

#include "Vulkan.hpp"

namespace usagi
{
class VulkanShaderModule
{
    VulkanUniqueShaderModule mShaderModule;
    std::unique_ptr<spirv_cross::Compiler> mReflectionCompiler;

public:
    VulkanShaderModule(
        VulkanUniqueShaderModule shader_module,
        std::unique_ptr<spirv_cross::Compiler> reflection_compiler)
        : mShaderModule(std::move(shader_module))
        , mReflectionCompiler(std::move(reflection_compiler))
    {
    }

    const VulkanUniqueShaderModule & shader_module() const
    {
        return mShaderModule;
    }

    const spirv_cross::Compiler & reflection() const
    {
        return *mReflectionCompiler;
    }

    // std::vector<vk::VertexInputBindingDescription>   vertex_inputs();
};
}
