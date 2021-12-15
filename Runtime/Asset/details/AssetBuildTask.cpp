#include "AssetBuildTask.hpp"

namespace usagi
{
// ReSharper disable once CppMemberFunctionMayBeConst
void AssetBuildTaskBase::update_asset_status(const AssetStatus status)
{
    mRecord->status.store(status, std::memory_order::release);
}

AssetBuildTaskBase::AssetBuildTaskBase(
    AssetManager2 &manager,
    TaskExecutor &executor,
    AssetRecord *record): mManager(manager)
    , mExecutor(executor)
    , mRecord(record)
{
}

std::shared_future<void> AssetBuildTaskBase::future()
{
    return mPromise.get_future();
}

bool AssetBuildTaskBase::precondition()
{
    // We should be the only instance processing the asset.
    const auto a = mRecord->status == AssetStatus::QUEUED;
    const auto b = mRecord->asset == nullptr;

    return a && b;
}

void AssetBuildTaskBase::on_started()
{
    update_asset_status(AssetStatus::LOADING);
}

void AssetBuildTaskBase::on_finished()
{
    if(mRecord->asset)
        update_asset_status(AssetStatus::READY);
    else
        // todo: fail reason?
        update_asset_status(AssetStatus::FAILED);
    mPromise.set_value();
}

bool AssetBuildTaskBase::postcondition()
{
    if(mRecord->status == AssetStatus::READY)
        return mRecord->asset != nullptr;
    if(mRecord->status == AssetStatus::FAILED)
        return true;
    return false;
}
}
