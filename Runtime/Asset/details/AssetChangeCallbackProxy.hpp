#pragma once

#include "../AssetPath.hpp"

namespace usagi
{
class AssetPackage;
class AssetManager2;

class AssetChangeCallbackProxy
{
    AssetManager2 *mManager = nullptr;

public:
    explicit AssetChangeCallbackProxy(AssetManager2 *manager);

    bool unload_asset(
        AssetPath path,
        AssetPackage *from_package = nullptr) const;
    void unload_subtree(
        AssetPath path,
        AssetPackage *from_package = nullptr) const;
    void unload_package_assets(AssetPackage *from_package) const;
    void unload_overriden_asset(AssetPath path) const;
    void unload_overriden_subtree(AssetPath path) const;
};
}
