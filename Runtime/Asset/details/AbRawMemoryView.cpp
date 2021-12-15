#include "AbRawMemoryView.hpp"

#include <Usagi/Library/Memory/MemoryArena.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetManager2.hpp>

#include "AssetQuery.hpp"

namespace usagi
{
AbRawMemoryView::AbRawMemoryView(const AssetPath path)
    : mCleanAssetPath(path.reconstructed())
{
}

std::unique_ptr<AssetRawMemoryView> AbRawMemoryView::construct_with(
    AssetManager2 &asset_manager,
    TaskExecutor &executor)
{
    /*
     * Locate the asset package containing the requested asset following
     * overriding rule. See AssetPackageManager for details.
     */
    MemoryArena arena;
    // The query goes through asset manager to have proper synchronization.
    AssetQuery *query = asset_manager.create_asset_query(
        { mCleanAssetPath },
        arena
    );
    USAGI_ASSERT_THROW(
        query,
        std::runtime_error(std::format(
            "Asset could not be found: ", mCleanAssetPath
        ))
    );

    /*
     * Preload the bytes into memory so that subsequent reads will unlikely
     * cause page faults.
     * todo: partial load?
     * todo: hash content?
     */
    query->fetch();
    assert(query->ready());

    return std::make_unique<AssetRawMemoryView>(query->data());
}
}
