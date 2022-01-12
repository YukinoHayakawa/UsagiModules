#pragma once

#include <vector>
#include <memory>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "../AssetPackage.hpp"

namespace usagi
{
class AssetQuery;
class AssetManager2;

// Note: externally synced.
class AssetPackageManager : Noncopyable
{
    std::vector<std::unique_ptr<AssetPackage>> mPackages;

public:
    using PackageRef = decltype(mPackages)::iterator;

    // todo: adding/removing an asset package should not happen while there is any active asset loading task, or during any request of assets. because loading tasks may hold references to asset entries and changes in packages may invalidate them.
    PackageRef add_package(std::unique_ptr<AssetPackage> package);
    void remove_package(AssetPackage *package);
    void remove_package(PackageRef package);

    // void visit_packages(auto visitor)
    // {
    //     for(auto &&pkg : std::ranges::reverse_view(mPackages))
    //         visitor(pkg.get());
    // }

    // Locate the entry in loaded packages.
    /**
     * \brief Locate the first package containing the requested asset, starting
     * from the latest added package to the oldest added one. Return true if
     * the asset was found. Note that even if the asset was found, it is not
     * guaranteed that the asset could be successfully loaded, for example when
     * the corresponding file was removed just between the query and the
     * loading request.
     * \param path Asset path. Should not begin or end with a slash.
     * \param arena A temporary memory area for constructing the query object
     * implemented by packages.
     * \return true if the asset was found.
     */
    ReturnValue<AssetStatus, AssetQuery *> create_query(
        AssetPath path,
        MemoryArena &arena);

    void poll_asset_changes(AssetChangeCallbackProxy &callback);

protected:
    PackageRef locate_package(
        AssetPackage *package,
        bool throw_if_not_found);
};
}
