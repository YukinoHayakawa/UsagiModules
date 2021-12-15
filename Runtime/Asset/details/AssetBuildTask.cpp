#include "AssetBuildTask.hpp"

#include "AssetBuilder.hpp"
#include "../AssetManager2.hpp"

namespace usagi
{
// ReSharper disable once CppMemberFunctionMayBeConst
void AssetBuildTaskBase::update_asset_status(const AssetStatus status)
{
    mPromisedStatus.store(status, std::memory_order::release);
}

AssetBuildTaskBase::AssetBuildTaskBase(
    AssetManager2 &manager,
    TaskExecutor &executor,
    std::unique_ptr<Asset> &promised_asset,
    std::atomic<AssetStatus> &promised_status)
    : mManager(manager)
    , mExecutor(executor)
    , mPromisedAsset(promised_asset)
    , mPromisedStatus(promised_status)
{
}

std::shared_future<void> AssetBuildTaskBase::future()
{
    return mPromise.get_future();
}

bool AssetBuildTaskBase::precondition()
{
    // We should be the only instance processing the asset.
    const auto a = mPromisedStatus == AssetStatus::QUEUED;
    const auto b = mPromisedAsset == nullptr;

    return a && b;
}

void AssetBuildTaskBase::on_started()
{
    update_asset_status(AssetStatus::LOADING);
}

void AssetBuildTaskBase::on_finished()
{
    if(mPromisedAsset)
        update_asset_status(AssetStatus::READY);
    else
        // todo: fail reason?
        update_asset_status(AssetStatus::FAILED);
    mPromise.set_value();
}

bool AssetBuildTaskBase::postcondition()
{
    if(mPromisedStatus == AssetStatus::READY)
        return mPromisedAsset != nullptr;
    if(mPromisedStatus == AssetStatus::FAILED)
        return true;
    return false;
}
}
