#pragma once

#include <Usagi/Library/Utility/ArgumentStorage.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapAssetManager.hpp"

namespace usagi
{
// bug this is temp impl
class RbAssetMemoryView : ArgumentStorage<AssetPath>
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapAssetManager;
    using ProductT = ReadonlyMemoryView;

    ResourceState construct(
        ResourceConstructDelegate<RbAssetMemoryView> &delegate);
};
}
