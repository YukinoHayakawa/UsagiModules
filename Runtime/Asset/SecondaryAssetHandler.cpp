#include "SecondaryAssetHandler.hpp"

#include <cassert>

#include <Usagi/Library/Memory/RawResource.hpp>

#include "AssetManager.hpp"

namespace usagi
{
std::shared_future<PrimaryAssetMeta>
SecondaryAssetHandlerBase::primary_asset_async(std::string_view asset_path)
{
    assert(mManager);
    assert(mExecutor);

    return mManager->primary_asset_async(asset_path, *mExecutor);
}

std::shared_future<SecondaryAssetMeta>
SecondaryAssetHandlerBase::secondary_asset_async(
    std::unique_ptr<SecondaryAssetHandlerBase> handler)
{
    assert(mManager);
    assert(mExecutor);

    return mManager->secondary_asset_async(
        std::move(handler), *mExecutor, this);
}

std::unique_ptr<SecondaryAsset> SecondaryAssetHandlerBase::construct_with(
    AssetManager &asset_manager,
    TaskExecutor &work_queue)
{
    // Note that construct() may throw, so use RAII helpers to manager the
    // assignments.
    RawResource a {
        [&]() { mManager = &asset_manager; },
        [&]() { mManager = nullptr; }
    };
    RawResource b {
        [&]() { mExecutor = &work_queue; },
        [&]() { mExecutor = nullptr; }
    };
    return construct();
}
}
