#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "AssetManager.hpp"

namespace usagi
{

void AssetManager::add_package(std::unique_ptr<AssetPackage> package)
{
    const auto pkg = package.get();
    auto type = pkg->type();
    auto root = pkg->root();

    // Locks all the mutexes since we may erase asset entries.
    // (scoped_lock has a certain way of preventing deadlocks)
    std::scoped_lock lks(
        mPackageMutex,
        mDependencyGraphMutex,
        mPrimaryAssetTableMutex,
        mSecondaryAssetTableMutex);

    // Allocate a vertex on the dependency graph for the package.
    const auto idx = mDependencyGraph.add_vertex();
    auto &v = mDependencyGraph.vertex(idx);

    auto it_pkg = mPackageManager.add_package(std::move(package), idx);
    v = it_pkg;

    LOG(info, "[Asset] Adding {} package: \"{}\" {{v={}}}", type, root, idx);


    // Scan over all loaded assets and invalidate any overridden assets.
    for(auto iter = mPrimaryAssets.begin(); iter != mPrimaryAssets.end();)
    {
        auto &[path, meta] = *iter;
        StackPolymorphicObject<AssetQuery> query;
        // The new package overrides a loaded primary asset.
        if(pkg->create_query(path, query))
        {
            LOG(info, "[Asset] Asset is overridden: \"{}\"", path);
            iter = invalidate_primary_asset_nolock(iter);
            continue;
        }
        ++iter;
    }
}

void AssetManager::remove_package(AssetPackage *package)
{
    std::unique_lock lk(mPackageMutex);

    auto it = mPackageManager.locate_package(package, true);

    mDependencyGraph.dfs_delete
    //
    it->vertex
}
}
