#include "AssetPackageManager.hpp"

#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
AssetPackageManager::PackageRef AssetPackageManager::add_package(
    std::unique_ptr<AssetPackage> package,
    const std::uint64_t vertex)
{
    PackageEntry pkg_meta;
    pkg_meta.package = std::move(package);
    pkg_meta.vertex = vertex;
    const auto iter = mPackages.emplace(mPackages.end(), std::move(pkg_meta));
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
        return v.package.get();
    });

    USAGI_ASSERT_THROW(
        throw_if_not_found || it_pkg != mPackages.end(),
        std::runtime_error("Package not found.")
    );

    return it_pkg;
}

AssetQuery * AssetPackageManager::create_query(
    const AssetPath path,
    MemoryArena &arena)
{
    AssetQuery *query = nullptr;
    // search in reverse order of added packages, so that packages added later
    // override those added earlier.
    for(const auto &[package, _] : std::ranges::reverse_view(mPackages))
        if((query = package->create_query(path, arena)))
            return query;

    return nullptr;
}
}
