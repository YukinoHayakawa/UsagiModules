#include "AssetManager2.hpp"

#include <format>

#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "details/AbRawMemoryView.hpp"
#include "details/AssetRawMemoryView.hpp"

namespace usagi
{
bool AssetManager2::unload_derivative_assets_nolock(
    AssetPath asset,
    AssetPackage *from_package)
{
    // Get package
    // todo should not couple with AbRawMemoryView
    const auto asset_id = build_asset_id<AbRawMemoryView>(asset);
    const auto asset_it = mAssetRecords.find(asset_id);

    if(asset_it == mAssetRecords.end())
    {
        LOG(debug, "[Asset] Asset {} not loaded. Aborting unload operation.",
            asset.reconstructed()
        );
        return false;
    }

    auto *asset_ptr = dynamic_cast<AssetRawMemoryView *>(
        asset_it->second.asset.get());
    assert(asset_ptr && "If the asset has a path, it must have be loaded as a "
        "memory view");

    if(from_package && asset_ptr->package() != from_package)
    {
        LOG(debug, "[Asset] Asset {} not loaded from {}. "
            "Aborting unload operation.",
            asset.reconstructed(),
            (void *)from_package);
        return false;
    }

    unload_derivative_assets_nolock(asset_id);

    return true;
}

void AssetManager2::unload_derivative_assets_nolock(AssetHashId asset)
{
    LOG(debug, "[Asset] Unloading asset and derivatives: {:#0x}", asset);

    USAGI_ASSERT_THROW(
        mAssetRecords.contains(asset),
        std::runtime_error(std::format(
            "[Asset] Asset to be reloaded doesn't exist: {:#0x}", asset
        ))
    );
    // Unload assets depending on the current one.
    unload_derivative_assets_nolock_helper(asset);
}

void AssetManager2::unload_derivative_assets_nolock(AssetPackage *package)
{
    LOG(debug, "[Asset] Unloading assets loaded from package {} "
        "and derivatives.", (void *)package);

    // Unload assets depending on the current package.
    unload_derivative_assets_nolock_helper(package, true);
}

void AssetManager2::unload_derivative_assets_nolock_helper(
    Vertex vertex,
    const bool is_package)
{
    // Recursively unload all derivative assets.

    // Get all outgoing edges.

    // Leaf assets do not have edges to other assets.
    if(const auto out_edges = mDependencies.find(vertex);
        out_edges != mDependencies.end())
    {
        // If the asset points to other assets, the assets being pointed to are
        // depending on the current asset, unload them first. (Note that they
        // should not hold any reference to the current asset.)
        while(!out_edges->second.empty())
        {
            const auto &to = *out_edges->second.begin();
            assert(std::holds_alternative<AssetHashId>(to));
            const auto to_id = std::get<AssetHashId>(to);
            if(std::holds_alternative<AssetPackage *>(vertex))
                LOG(debug, "[Asset] Unloading derivative: {} -> {:#0x}",
                    (void *)std::get<AssetPackage *>(vertex), to_id);
            else
                LOG(debug, "[Asset] Unloading derivative: {:#0x} -> {:#0x}",
                    std::get<AssetHashId>(vertex), to_id);
            unload_derivative_assets_nolock_helper(to_id);
        }
        mDependencies.erase(out_edges);
    }

    if(!is_package)
    {
        assert(std::holds_alternative<AssetHashId>(vertex));
        const auto id = std::get<AssetHashId>(vertex);

        // Erase incoming edges.

        // First, find out which vertices point to the current asset.
        const auto it_rev = mDependencyReversed.find(vertex);
        assert(it_rev != mDependencyReversed.end());

        // Remove their edges to here.
        for(auto &&in : it_rev->second)
        {
            erase_dependency_edge_nolock(in, id);
        }

        // Delete the reverse edges.
        mDependencyReversed.erase(it_rev);

        unload_single_asset_nolock(id);
    }
    else
    {
        assert(!mDependencyReversed.contains(vertex) &&
            "Asset package should not have incoming edges.");
    }
}
}
