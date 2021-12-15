#pragma once

#include <future>

#include <Usagi/Runtime/Task.hpp>

#include "AssetEnum.hpp"

namespace usagi
{
class Asset;
class AssetBuilder;

class AssetBuildTask final : public Task
{
    class AssetManager2 &mManager;
    class TaskExecutor &mExecutor;
    std::unique_ptr<Asset> &mPromisedAsset;
    std::atomic<AssetStatus> &mPromisedStatus;
    std::unique_ptr<AssetBuilder> mBuilder;
    std::promise<void> mPromise;

    void update_asset_status(AssetStatus status);

public:
    AssetBuildTask(
        AssetManager2 &manager,
        TaskExecutor &executor,
        std::unique_ptr<Asset> &promised_asset,
        std::atomic<AssetStatus> &promised_status,
        std::unique_ptr<AssetBuilder> builder);

    // Used for notifying that the asset build task is finished (whether
    // succeeded or failed)
    std::shared_future<void> future();

    bool precondition() override;
    void on_started() override;
    void run() override;
    void on_finished() override;
    bool postcondition() override;
};
}
