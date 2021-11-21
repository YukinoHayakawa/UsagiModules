#include "SahGlslCompiler.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/IO/Graphics/ShaderCompiler.hpp>

namespace usagi
{

SahGlslCompiler::SahGlslCompiler(std::string asset_path, GpuShaderStage stage)
    : SingleDependencySecondaryAssetHandler(std::move(asset_path))
    , mStage(stage)
{
}

std::unique_ptr<SecondaryAsset> SahGlslCompiler::construct()
{
    const auto src = await_depending_primary();

    LOG(info, "[glslang] Compiling {} shader: {}",
        to_string(mStage), asset_path());

    auto bytecodes = spirv::from_glsl(src.to_string_view(), mStage);

    return std::make_unique<SaSpirvBinary>(std::move(bytecodes));
}

void SahGlslCompiler::append_features(Hasher &hasher)
{
    SingleDependencySecondaryAssetHandler::append_features(hasher);

    hasher.append(to_string(mStage));
}
}
