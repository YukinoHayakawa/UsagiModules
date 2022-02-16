#pragma once

#include <Usagi/Library/Memory/MemoryArena.hpp>
#include <Usagi/Runtime/ReturnValue.hpp>

#include "AssetAccessProxy.hpp"
#include "AssetBuilder.hpp"
#include "../AssetPath.hpp"

namespace usagi
{
class AssetQuery;
struct AssetRecord;
class TaskExecutor;
class AssetManager2;

class AssetRequestProxy
{
	AssetManager2 *mManager = nullptr;
    TaskExecutor *mExecutor = nullptr;
    AssetHashId mRequester = 0;

public:
    AssetRequestProxy(
        AssetManager2 *manager,
        TaskExecutor *executor,
        AssetHashId requester);

    template <AssetBuilder Builder, typename... Args>
    AssetAccessProxy<typename AssetBuilderProductType<Builder>::ProductT>
        asset(Args &&...args) const
    {
        return mManager->asset<Builder>(
            mRequester,
            *mExecutor,
            std::forward<Args>(args)...
        );
    }

    ReturnValue<AssetStatus, AssetQuery *> create_asset_query(
        AssetPath path,
        MemoryArena &arena) const;
};
}
