#pragma once

#include <memory>

#include "AssetRequestProxy.hpp"
#include "AssetRawMemoryView.hpp"

namespace usagi
{
/*
 * Build the most primitive type of asset: a memory view, which only creates
 * a dependency on the asset package provided the memory view.
 * todo: load only a part of an asset without introducing additional asset entries
 */
class AbRawMemoryView
{
    // Store a copy of the asset path because we can't manage the lifetime
    // of the one passed to the ctor.
    std::string mCleanAssetPath;

public:
    explicit AbRawMemoryView(AssetPath path);

    ReturnValue<AssetStatus, std::unique_ptr<AssetRawMemoryView>>
    construct_with(AssetRequestProxy request_proxy) const;
};

static_assert(AssetBuilder<AbRawMemoryView>);
}
