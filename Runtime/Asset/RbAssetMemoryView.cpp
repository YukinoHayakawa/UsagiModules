#include "RbAssetMemoryView.hpp"

#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

#include "AssetManager2.hpp"
#include "details/AssetQuery.hpp"

namespace usagi
{
ResourceState RbAssetMemoryView::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &asset_path)
{
    /*
     * Locate the asset package containing the requested asset following
     * overriding rule. See AssetPackageManager for details.
     */
    // todo track which resource is requesting from the heap
    auto *asset_manager = delegate.heap<AssetManager2>();

    MemoryArena arena;
    // The query goes through asset manager which sync the accesses.
    auto [status, query] = asset_manager->create_asset_query(
        0,
        asset_path.reconstructed(),
        arena
    );

    if(status == AssetStatus::MISSING)
        return ResourceState::FAILED_INVALID_DATA;
    if(status == AssetStatus::EXIST_BUSY)
        return ResourceState::FAILED_BUSY;
    if(status == AssetStatus::EXIST)
    {
        query->fetch();
        assert(query->ready());

        delegate.emplace(
            query->data(),
            [] {
                // todo: notify the asset manager to release asset
            }
        );

        return ResourceState::READY;
    }

    USAGI_UNREACHABLE("Invalid asset status.");
}
}
