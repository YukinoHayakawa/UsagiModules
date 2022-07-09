#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Runtime/Memory/View.hpp>

#include "AssetPath.hpp"

namespace usagi
{
class RbAssetMemoryView : public ResourceBuilderDecl<
    ReadonlyMemoryView,
    const AssetPath &>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &asset_path) override;
};
}
