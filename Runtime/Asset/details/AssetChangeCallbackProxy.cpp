#include "AssetChangeCallbackProxy.hpp"

#include <Usagi/Modules/Runtime/Asset/AssetManager2.hpp>

namespace usagi
{
AssetChangeCallbackProxy::AssetChangeCallbackProxy(AssetManager2 *manager)
    : mManager(manager)
{
}

bool AssetChangeCallbackProxy::unload_asset(
    AssetPath path,
    AssetPackage *from_package) const
{
    return mManager->unload_derivative_assets_nolock(path, from_package);
}

void AssetChangeCallbackProxy::unload_subtree(
    AssetPath path,
    AssetPackage *from_package) const
{
    mManager->unload_subtree(path, from_package);
}

void AssetChangeCallbackProxy::unload_package_assets(
    AssetPackage *from_package) const
{
    mManager->unload_package_assets(from_package);
}

void AssetChangeCallbackProxy::unload_overriden_asset(AssetPath path) const
{
    mManager->unload_overriden_asset(path);
}

void AssetChangeCallbackProxy::unload_overriden_subtree(AssetPath path) const
{
    mManager->unload_overriden_subtree(path);
}
}
