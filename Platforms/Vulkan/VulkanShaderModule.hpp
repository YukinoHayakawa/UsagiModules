#pragma once

#include <spirv_cross/spirv_cross.hpp>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Library/Memory/Nonmovable.hpp>

#include "Vulkan.hpp"

namespace usagi
{
class VulkanShaderModule : Noncopyable, Nonmovable
{
    VulkanUniqueShaderModule mShaderModule;
    // Contains the info that supports runtime reflection of the shader.
    spirv_cross::Compiler mReflectionCompiler; // bug this is very big. store as ptr?

public:
    VulkanShaderModule(
        VulkanUniqueShaderModule shader_module,
        std::vector<std::uint32_t> bytecode_copy)
        : mShaderModule(std::move(shader_module))
        , mReflectionCompiler(std::move(bytecode_copy))
    {
    }

    const VulkanUniqueShaderModule & shader_module() const
    {
        return mShaderModule;
    }

    const spirv_cross::Compiler & reflection() const
    {
        return mReflectionCompiler;
    }

    // std::vector<vk::VertexInputBindingDescription>   vertex_inputs();
};
}
