#include "AssetBuildTask.hpp"

#include "AssetBuilder.hpp"
#include "../AssetManager2.hpp"

namespace usagi
{
// ReSharper disable once CppMemberFunctionMayBeConst
void AssetBuildTask::update_asset_status(const AssetStatus status)
{
    mPromisedStatus.store(status, std::memory_order::release);
}

AssetBuildTask::AssetBuildTask(
    AssetManager2 &manager,
    TaskExecutor &executor,
    std::unique_ptr<Asset> &promised_asset,
    std::atomic<AssetStatus> &promised_status,
    std::unique_ptr<AssetBuilder> builder): mManager(manager)
    , mExecutor(executor)
    , mPromisedAsset(promised_asset)
    , mPromisedStatus(promised_status)
    , mBuilder(std::move(builder))
{
}

std::shared_future<void> AssetBuildTask::future()
{
    return mPromise.get_future();
}

bool AssetBuildTask::precondition()
{
    // We should be the only instance processing the asset.
    const auto a = mPromisedStatus == AssetStatus::QUEUED;
    const auto b = mPromisedAsset == nullptr;

    return a && b;
}

void AssetBuildTask::on_started()
{
    update_asset_status(AssetStatus::LOADING);
}

void AssetBuildTask::run()
{
    // todo: hash dependency content?
    // XXHash64 hasher(0);

    // mEntry->meta().fingerprint_dep_content = hasher.hash();
    // mPromise.set_value(mEntry->meta());

    // Build the asset
    auto object = mBuilder->construct_with(mManager, mExecutor);
    // todo verify the type of returned object is the same as declared by SecondaryAssetT
    mPromisedAsset = std::move(object);

    // mEntry->fingerprint_dep_content = hasher.hash();
}

void AssetBuildTask::on_finished()
{
    if(mPromisedAsset)
        update_asset_status(AssetStatus::READY);
    else
        // todo: fail reason?
        update_asset_status(AssetStatus::FAILED);
    mPromise.set_value();
}

bool AssetBuildTask::postcondition()
{
    if(mPromisedStatus == AssetStatus::READY)
        return mPromisedAsset != nullptr;
    if(mPromisedStatus == AssetStatus::FAILED)
        return true;
    return false;
}
}
