#pragma once

#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetDerivative.hpp>
#include <Usagi/Modules/IO/Graphics/Enum.hpp>

#include "SpirvBytecodes.hpp"

namespace usagi
{
class RbSpirvBytecodes : RbAssetDerivative
{
    GpuShaderStage mStage;

public:
    RbSpirvBytecodes(std::string normalized_asset_path, GpuShaderStage stage);

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = SpirvBytecodes;

    // todo error handling
    ResourceState construct(
        ResourceConstructDelegate<RbSpirvBytecodes> &delegate);
};
}
