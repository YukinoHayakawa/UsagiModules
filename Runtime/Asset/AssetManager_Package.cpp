#include "AssetManager2.hpp"

namespace usagi
{
void AssetManager2::add_package(std::unique_ptr<AssetPackage> package)
{
    std::scoped_lock lk(mPackageMutex, mAssetTableMutex, mDependencyGraphMutex);
    // todo unload_overriden_assets(package);
    mPackageManager.add_package(std::move(package));
}

void AssetManager2::remove_package(AssetPackage *package)
{
    std::scoped_lock lk(mPackageMutex, mAssetTableMutex, mDependencyGraphMutex);
    // todo: it's generally impossible to make sure all affected assets are not in use
    // todo unload_derivative_assets_nolock(package);
    mPackageManager.remove_package(package);
}
}
