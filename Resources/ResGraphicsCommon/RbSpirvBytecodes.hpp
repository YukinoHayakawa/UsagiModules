#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>

namespace usagi
{
class RbSpirvBytecodes : public ResourceBuilderDecl<
    std::vector<std::uint32_t>,
    const AssetPath &,
    GpuShaderStage>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path,
        GpuShaderStage stage) override;
};
}
