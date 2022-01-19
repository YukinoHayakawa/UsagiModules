#include "RbSpirvBytecodes.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/IO/Graphics/ShaderCompiler.hpp>

namespace usagi
{
RbSpirvBytecodes::RbSpirvBytecodes(
    std::string normalized_asset_path,
    GpuShaderStage stage)
    : RbAssetDerivative(std::move(normalized_asset_path))
    , mStage(stage)
{
}

ResourceState RbSpirvBytecodes::construct(
    ResourceConstructDelegate<RbSpirvBytecodes> &delegate)
{
    LOG(info,
        "[glslang] Compiling {} shader: {}",
        to_string(mStage),
        asset_path()
    );

    const auto shader_source = delegate.resource<
        RbAssetMemoryView
    >(asset_path()).await();

    auto bytecodes = spirv::from_glsl(
        shader_source.to_string_view(),
        mStage
    );

    delegate.allocate(std::move(bytecodes));

    return ResourceState::READY;
}
}
