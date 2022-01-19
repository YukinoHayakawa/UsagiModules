#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapAssetManager.hpp"

namespace usagi
{
// bug this is temp impl
class RbAssetMemoryView
{
    std::string mNormalizedAssetPath;

public:
    explicit RbAssetMemoryView(std::string normalized_asset_path);

    using TargetHeapT = HeapAssetManager;
    using ProductT = ReadonlyMemoryView;

    ResourceState construct(
        ResourceConstructDelegate<RbAssetMemoryView> &delegate);
};
}
