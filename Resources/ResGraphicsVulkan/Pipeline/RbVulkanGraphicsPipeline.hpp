#pragma once

#include <Usagi/Modules/Platforms/Vulkan/VulkanDeviceAccess.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

namespace usagi
{
class RbVulkanGraphicsPipeline
    : public ResourceBuilderDecl<
        VulkanGraphicsPipeline,
        const AssetPath &>
    , public VulkanDeviceAccess
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path) override;
};
}
