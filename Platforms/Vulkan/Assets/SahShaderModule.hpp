#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "ShaderModule.hpp"

namespace usagi
{
class VulkanGpuDevice;

class SahShaderModule : public SecondaryAssetHandler<ShaderModule>
{
    VulkanGpuDevice *mDevice = nullptr;
    std::string mAssetPath, mStage;

public:
    SahShaderModule(
        VulkanGpuDevice *device,
        std::string asset_path,
        std::string stage);

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
