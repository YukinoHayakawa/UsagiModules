#pragma once

#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapFreeObjectManager.hpp>

#include "JsonDocument.hpp"

namespace usagi
{
class RbCascadingJsonConfig : ArgumentStorage<AssetPath>
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapFreeObjectManager;
    using ProductT = JsonDocument;

    ResourceState construct(
        ResourceConstructDelegate<RbCascadingJsonConfig> &delegate);
};
}
