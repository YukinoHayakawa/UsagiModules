#pragma once

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>

#include "JsonDocument.hpp"

namespace usagi
{
class RbCascadingJsonConfig
{
    std::string mNormalizedAssetPath;

public:
    explicit RbCascadingJsonConfig(std::string normalized_asset_path);

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = JsonDocument;

    ResourceState construct(
        ResourceConstructDelegate<RbCascadingJsonConfig> &delegate);
};
}
