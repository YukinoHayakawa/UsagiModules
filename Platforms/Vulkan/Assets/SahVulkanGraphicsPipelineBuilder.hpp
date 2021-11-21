#pragma once

#include <nlohmann/json_fwd.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

namespace usagi
{
class VulkanGpuDevice;
class VulkanGraphicsPipeline;

class SahVulkanGraphicsPipelineBuilder
    : public SingleDependencySecondaryAssetHandler<VulkanGraphicsPipeline>
{
    VulkanGpuDevice *mDevice = nullptr;

public:
    SahVulkanGraphicsPipelineBuilder(
        VulkanGpuDevice *device,
        std::string asset_path);

protected:
    std::unique_ptr<SecondaryAsset> construct() override;

    std::pair<std::shared_future<SecondaryAssetMeta>, std::string>
    async_shader_module(
        const nlohmann::json &obj,
        const char *key,
        std::string_view stage);
};
}
