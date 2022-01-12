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
    AssetHashId id,
    AssetRecord *record): mManager(manager)
    , mExecutor(executor)
    , mId(id)
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
    update_asset_status(mStatus);
    mPromise.set_value();
}

bool AssetBuildTaskBase::postcondition()
{
    switch(mStatus)
    {
        case AssetStatus::MISSING:
        case AssetStatus::MISSING_DEPENDENCY:
        case AssetStatus::EXIST_BUSY:
        case AssetStatus::FAILED:
            return mRecord->asset == nullptr;
        case AssetStatus::READY:
            return mRecord->asset != nullptr;
        default: return false;
    }
}
}
