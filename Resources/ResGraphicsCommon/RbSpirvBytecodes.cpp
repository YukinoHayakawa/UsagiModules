#include "RbSpirvBytecodes.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/IO/Graphics/ShaderCompiler.hpp>

namespace usagi
{
ResourceState RbSpirvBytecodes::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path,
    const GpuShaderStage stage)
{
    LOG(info,
        "[glslang] Compiling {} shader: {}",
        to_string(stage),
        path
    );

    const auto source_text = delegate.resource<RbAssetMemoryView>(path).await();

    auto bytecodes = spirv::from_glsl(
        source_text->to_string_view(),
        stage
    );

    delegate.emplace(std::move(bytecodes));

    return ResourceState::READY;
}
}
