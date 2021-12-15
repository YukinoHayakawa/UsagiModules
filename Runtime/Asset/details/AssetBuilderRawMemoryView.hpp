#pragma once

#include "AssetBuilder.hpp"
#include "AssetPath.hpp"

namespace usagi
{
/*
 * Build the most primitive type of asset: a memory view, which only creates
 * a dependency on the asset package provided the memory view.
 * todo: load only a part of an asset without introducing additional asset entries
 */
class AssetBuilderRawMemoryView : public AssetBuilder
{
    // Store a copy of the asset path because we can't manage the lifetime
    // of the one passed to the ctor.
    std::string mCleanAssetPath;

public:
    using ProductT = AssetRawMemoryView;

    explicit AssetBuilderRawMemoryView(AssetPath path);

    std::unique_ptr<Asset> construct_with(
        AssetManager2 &asset_manager,
        TaskExecutor &executor) override;
};
static_assert(IsProperAssetBuilder<AssetBuilderRawMemoryView>);
}
