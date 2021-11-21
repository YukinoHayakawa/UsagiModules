#pragma once

#include <spirv_cross/spirv_cross.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>
#include <Usagi/Modules/Platforms/Vulkan/Vulkan.hpp>

namespace usagi
{
class ShaderModule : public SecondaryAsset
{
    VulkanUniqueShaderModule mShaderModule;
    std::unique_ptr<spirv_cross::Compiler> mReflectionCompiler;

public:
    ShaderModule(
        VulkanUniqueShaderModule shader_module,
        std::unique_ptr<spirv_cross::Compiler> reflection_compiler)
        : mShaderModule(std::move(shader_module))
        , mReflectionCompiler(std::move(reflection_compiler))
    {
    }

    const auto & value() const
    {
        return *this;
    }

    const VulkanUniqueShaderModule & shader_module() const
    {
        return mShaderModule;
    }

    const std::unique_ptr<spirv_cross::Compiler> & reflection() const
    {
        return mReflectionCompiler;
    }
};
}
