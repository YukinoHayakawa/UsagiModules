#include "AssetManager2.hpp"

#include "details/AbRawMemoryView.hpp"
#include "details/AssetChangeCallbackProxy.hpp"
#include "details/AssetQuery.hpp"

// This compilation unit contains those functions exposed to the user.
namespace usagi
{
ReturnValue<AssetStatus, AssetQuery *> AssetManager2::create_asset_query(
    AssetHashId requester,
    AssetPath path,
    MemoryArena &arena)
{
    // Note: although iterating through the packages is a read operation,
    // the query can cause modifications to certain package and that
    // is synchronized internally within the package impl.
    std::scoped_lock lk(
        mPackageMutex,          // Read packages
        mDependencyGraphMutex   // May write dependency edge
    );
    auto [status, query] = mPackageManager.create_query(path, arena);
    if(status == AssetStatus::EXIST && requester)
    {
        assert(query && "If the asset exists, "
            "the query object must not be null.");
        register_dependency_nolock(requester, query->package());
    }
    return { status, query };
}

void AssetManager2::poll_asset_changes()
{
    /*
     * Possible choices:
     * 1. Poll asset state from asset packages
     *   - How:  For each asset, query the package where it was loaded from
     *           about whether the asset is updated, using its asset path.
     *   - Pros:
     *   - Cons: Cannot loop through the assets since the map will be modified
     *           by the callback.
     *           Asset change events may pile up in packages. (Poll mod time
     *           instead?)
     * 2. Let asset packages callback.
     *   - How:  For each package, provide the package a method that it can
     *           call when there is an asset is changed.
     *   - Pros: Probably more efficient performance wise.
     *           Easier to extend to other models such as network synced assets.
     *   - Cons: A package doesn't know whether an asset is loaded from it.
     * Currently using the second strategy.
     */
    std::scoped_lock lk(mPackageMutex, mAssetTableMutex, mDependencyGraphMutex);
    auto callback = AssetChangeCallbackProxy(this);
    mPackageManager.poll_asset_changes(callback);
}


void AssetManager2::unload_subtree(AssetPath path, AssetPackage *from_package)
{
// todo
    /*
    auto prefix = path.reconstructed();
    if(prefix.empty()) return false;
    // Make sure the path is a folder path
    if(prefix.back() != '/') prefix.push_back('/');

    while(true)
    {
        auto it = mPathIdMapping.lower_bound(prefix);
        if(it == mPathIdMapping.end()) break;
        if(!it->first.starts_with(prefix)) break;
        auto proxy = asset<AbRawMemoryView>(it->second);
        assert(proxy.status() == AssetStatus::READY &&
            "The asset to be unloaded must be in memory.");
        unload_derivative_assets(it->second, proxy.maybe_asset()->package());
    }*/
}

void AssetManager2::unload_package_assets(AssetPackage *from_package)
{
    // todo
    // unload_derivative_assets(from_package);
}

void AssetManager2::unload_overriden_asset(AssetPath path)
{
    // todo
}

void AssetManager2::unload_overriden_subtree(AssetPath path)
{
    // todo
}

AssetManager2::AssetRecordRef AssetManager2::find_asset_nolock(AssetHashId id)
{
    const auto it = mAssetRecords.find(id);
    return it;
}

AssetManager2::AssetRecordRef AssetManager2::find_asset(AssetHashId id)
{
    std::shared_lock lk(mAssetTableMutex);
    return find_asset_nolock(id);
}
}
