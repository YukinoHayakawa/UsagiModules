#pragma once

#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

namespace usagi
{
using SaSpirvBinary = SecondaryAssetAdapter<std::vector<std::uint32_t>>;

class SahGlslCompiler
    : public SingleDependencySecondaryAssetHandler<SaSpirvBinary>
{
    GpuShaderStage mStage;

public:
    SahGlslCompiler(std::string asset_path, GpuShaderStage stage);

    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
