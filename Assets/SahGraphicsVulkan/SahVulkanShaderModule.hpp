#pragma once

#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanShaderModule.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

namespace usagi
{
class VulkanGpuDevice;

using SaVulkanShaderModule = SecondaryAssetAdapter<VulkanShaderModule>;

class SahVulkanShaderModule
    : public SingleDependencySecondaryAssetHandler<SaVulkanShaderModule>
{
    VulkanGpuDevice *mDevice = nullptr;
    GpuShaderStage mStage;

public:
    SahVulkanShaderModule(
        VulkanGpuDevice *device,
        std::string asset_path,
        GpuShaderStage stage);

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
