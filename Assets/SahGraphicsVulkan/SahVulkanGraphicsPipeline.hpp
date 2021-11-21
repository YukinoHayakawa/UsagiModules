#pragma once

#include <nlohmann/json_fwd.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>
#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipeline.hpp>

namespace usagi
{
class VulkanGpuDevice;

using SaVulkanGraphicsPipeline = SecondaryAssetAdapter<VulkanGraphicsPipeline>;

class SahVulkanGraphicsPipeline
    : public SingleDependencySecondaryAssetHandler<SaVulkanGraphicsPipeline>
{
    VulkanGpuDevice *mDevice = nullptr;

public:
    SahVulkanGraphicsPipeline(
        VulkanGpuDevice *device,
        std::string asset_path);

protected:
    std::unique_ptr<SecondaryAsset> construct() override;

    std::pair<std::shared_future<SecondaryAssetMeta>, std::string>
    async_shader_module(
        const nlohmann::json &obj,
        const char *key,
        GpuShaderStage stage);
};
}
