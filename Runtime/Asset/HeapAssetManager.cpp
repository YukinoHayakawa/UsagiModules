#include "HeapAssetManager.hpp"

namespace usagi
{
ReturnValue<AssetStatus, AssetQuery *> HeapAssetManager::allocate(
    HeapResourceIdT id,
    AssetPath path,
    MemoryArena &arena)
{
    // todo don't manage dependencies
    auto query = create_asset_query(
        0, path, arena
    );

    if(query.code() == AssetStatus::EXIST)
    {
        auto [it, inserted] = mEntries.try_emplace(id);
        it->second.path = path.reconstructed();
        it->second.package = query.value()->package();
        assert(inserted);
    }

    return query;
}

ReadonlyMemoryView HeapAssetManager::resource(const HeapResourceIdT id)
{
    // todo has to record loaded assets
    const auto it = mEntries.find(id);
    assert(it != mEntries.end());
    MemoryArena arena;
    auto query = it->second.package->create_query(it->second.path, arena);
    assert(query.value());
    return query.value()->data();
}
}
