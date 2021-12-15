#pragma once

#include <Usagi/Library/Memory/MemoryArena.hpp>

#include "AssetAccessProxy.hpp"
#include "AssetBuilder.hpp"
#include "AssetPath.hpp"

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
    AssetRecord *mRecord = nullptr;

public:
    AssetRequestProxy(
        AssetManager2 *manager,
        TaskExecutor *executor,
        AssetRecord *record)
        : mManager(manager)
        , mExecutor(executor)
        , mRecord(record)
    {
    }

    template <AssetBuilder Builder, typename... Args>
    AssetAccessProxy<typename AssetBuilderProductType<Builder>::ProductT>
        asset(Args &&...args) const
    {
        return mManager->asset<Builder>(
            mRecord,
            *mExecutor,
            std::forward<Args>(args)...
        );
    }

    AssetQuery * create_asset_query(AssetPath path, MemoryArena &arena) const;
};
}
