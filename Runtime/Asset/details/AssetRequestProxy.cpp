#include "AssetRequestProxy.hpp"

#include <Usagi/Modules/Runtime/Asset/AssetManager2.hpp>

namespace usagi
{
AssetQuery * AssetRequestProxy::create_asset_query(
    AssetPath path,
    MemoryArena &arena) const
{
    return mManager->create_asset_query(mRecord, path, arena);
}
}
