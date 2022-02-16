#pragma once

#include <Usagi/Library/Utility/ArgumentStorage.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/IO/Graphics/Enum.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>

#include "SpirvBytecodes.hpp"

namespace usagi
{
class RbSpirvBytecodes
    : ArgumentStorage<
        AssetPath,
        GpuShaderStage
    >
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = SpirvBytecodes;

    // todo error handling
    ResourceState construct(
        ResourceConstructDelegate<RbSpirvBytecodes> &delegate);
};
}
