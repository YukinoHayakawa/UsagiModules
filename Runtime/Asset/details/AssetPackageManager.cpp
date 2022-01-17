#include "AssetPackageManager.hpp"

#include <Usagi/Runtime/ErrorHandling.hpp>

// #include "../AssetManager2.hpp"

namespace usagi
{
AssetPackageManager::PackageRef AssetPackageManager::add_package(
    std::unique_ptr<AssetPackage> package)
{
    const auto iter = mPackages.emplace(mPackages.end(), std::move(package));
    return iter;
}

void AssetPackageManager::remove_package(AssetPackage *package)
{
    const auto it = locate_package(package, true);
    remove_package(it);
}

void AssetPackageManager::remove_package(PackageRef package)
{
    mPackages.erase(package);
}

AssetPackageManager::PackageRef AssetPackageManager::locate_package(
    AssetPackage *package,
    const bool throw_if_not_found)
{
    // Locate the package in the dependency graph
    const auto it_pkg = std::ranges::find(mPackages, package, [](auto &&v) {
        return v.get();
    });

    USAGI_ASSERT_THROW(
        throw_if_not_found || it_pkg != mPackages.end(),
        std::runtime_error("Package not found.")
    );

    return it_pkg;
}

ReturnValue<AssetStatus, AssetQuery *> AssetPackageManager::create_query(
    const AssetPath path,
    MemoryArena &arena)
{
    // todo lock
    // search in reverse order of added packages, so that packages added later
    // override those added earlier.
    for(auto &&package : std::ranges::reverse_view(mPackages))
    {
        auto [status, query] = package->create_query(path, arena);
        if(status != AssetStatus::MISSING)
            return { status, query };
    }

    return { AssetStatus::MISSING, nullptr };
}

void AssetPackageManager::poll_asset_changes(
    AssetChangeCallbackProxy &callback)
{
    // std::size_t num_changed_assets = 0;
    for(auto &&package : std::ranges::reverse_view(mPackages))
        package->poll_asset_changes(callback);
    // return num_changed_assets;
}
}
