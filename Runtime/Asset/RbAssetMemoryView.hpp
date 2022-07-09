#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Runtime/Memory/View.hpp>

#include "AssetPath.hpp"

namespace usagi
{
// bug this is temp impl
class RbAssetMemoryView
{
public:
    using ProductT = ReadonlyMemoryView;
    using BuildArguments = std::tuple<AssetPath>;

    static ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &asset_path);
};
}
