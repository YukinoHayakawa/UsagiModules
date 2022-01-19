#pragma once

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>

#include "JsonDocument.hpp"

namespace usagi
{
class RbJsonDocument
{
    std::string mNormalizedAssetPath;

public:
    explicit RbJsonDocument(std::string normalized_asset_path);

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = JsonDocument;

    ResourceState construct(
        ResourceConstructDelegate<RbJsonDocument> &delegate);
};

static_assert(ResourceBuilder<RbJsonDocument>);
}
