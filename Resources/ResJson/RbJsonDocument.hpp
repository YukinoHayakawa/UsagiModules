#pragma once

#include <Usagi/Library/Utility/ArgumentStorage.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "JsonDocument.hpp"

namespace usagi
{
class RbJsonDocument : ArgumentStorage<AssetPath>
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = JsonDocument;

    ResourceState construct(
        ResourceConstructDelegate<RbJsonDocument> &delegate);
};
}
