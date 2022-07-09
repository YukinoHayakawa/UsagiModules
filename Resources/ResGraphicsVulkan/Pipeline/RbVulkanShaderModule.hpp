#pragma once

#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanDeviceAccess.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

namespace usagi
{
class RbVulkanShaderModule
    : public ResourceBuilderDecl<
        VulkanShaderModule,
        const AssetPath &,
        GpuShaderStage>
    , public VulkanDeviceAccess
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path,
        GpuShaderStage stage) override;
};
}
