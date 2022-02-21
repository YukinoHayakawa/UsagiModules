#include "RbSpirvBytecodes.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/IO/Graphics/ShaderCompiler.hpp>

namespace usagi
{
ResourceState RbSpirvBytecodes::construct(
    ResourceConstructDelegate<RbSpirvBytecodes> &delegate)
{
    LOG(info,
        "[glslang] Compiling {} shader: {}",
        to_string(arg<GpuShaderStage>()),
        arg<AssetPath>()
    );

    const auto shader_source = delegate.resource<
        RbAssetMemoryView
    >(arg<AssetPath>()).await();

    auto bytecodes = spirv::from_glsl(
        shader_source->to_string_view(),
        arg<GpuShaderStage>()
    );

    delegate.allocate(std::move(bytecodes));

    return ResourceState::READY;
}
}
