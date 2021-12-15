#pragma once

#include <future>

#include <Usagi/Runtime/Task.hpp>

#include "AssetEnum.hpp"
#include "AssetBuilder.hpp"

namespace usagi
{
class Asset;

class AssetBuildTaskBase : public Task
{
protected:
    class AssetManager2 &mManager;
    class TaskExecutor &mExecutor;
    std::unique_ptr<Asset> &mPromisedAsset;
    std::atomic<AssetStatus> &mPromisedStatus;
    std::promise<void> mPromise;

    void update_asset_status(AssetStatus status);

public:
    AssetBuildTaskBase(
        AssetManager2 &manager,
        TaskExecutor &executor,
        std::unique_ptr<Asset> &promised_asset,
        std::atomic<AssetStatus> &promised_status);

    // Used for notifying that the asset build task is finished (whether
    // succeeded or failed)
    std::shared_future<void> future();

    bool precondition() override;
    void on_started() override;
    void on_finished() override;
    bool postcondition() override;
};

template <AssetBuilder BuilderT>
class AssetBuildTask : public AssetBuildTaskBase
{
    std::unique_ptr<BuilderT> mBuilder;

public:
    AssetBuildTask(
        AssetManager2 &manager,
        TaskExecutor &executor,
        std::unique_ptr<Asset> &promised_asset,
        std::atomic<AssetStatus> &promised_status,
        std::unique_ptr<BuilderT> builder)
        : AssetBuildTaskBase(
            manager, executor, promised_asset, promised_status
        )
        , mBuilder(std::move(builder))
    {
    }

    void run() override
    {
        // todo: hash dependency content?
        mPromisedAsset = mBuilder->construct_with(mManager, mExecutor);
    }
};
}
