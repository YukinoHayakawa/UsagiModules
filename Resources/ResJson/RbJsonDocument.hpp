#pragma once

#include <Usagi/Modules/Runtime/Asset/RbAssetDerivative.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>

#include "JsonDocument.hpp"

namespace usagi
{
class RbJsonDocument : public RbAssetDerivative
{
public:
    explicit RbJsonDocument(std::string normalized_asset_path);

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = JsonDocument;

    ResourceState construct(
        ResourceConstructDelegate<RbJsonDocument> &delegate);
};
}
