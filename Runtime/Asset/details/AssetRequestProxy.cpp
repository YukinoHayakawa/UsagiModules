#include "AssetRequestProxy.hpp"

#include <Usagi/Modules/Runtime/Asset/AssetManager2.hpp>

namespace usagi
{
AssetRequestProxy::AssetRequestProxy(
    AssetManager2 *manager,
    TaskExecutor *executor,
    AssetHashId requester): mManager(manager)
    , mExecutor(executor)
    , mRequester(requester)
{
}

ReturnValue<AssetStatus, AssetQuery *> AssetRequestProxy::create_asset_query(
    AssetPath path,
    MemoryArena &arena) const
{
    return mManager->create_asset_query(mRequester, path, arena);
}
}
