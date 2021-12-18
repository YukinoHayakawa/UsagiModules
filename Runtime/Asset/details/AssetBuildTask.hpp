#pragma once

#include <future>

#include <Usagi/Runtime/Task.hpp>

#include "AssetEnum.hpp"
#include "AssetRecord.hpp"
#include "AssetRequestProxy.hpp"

namespace usagi
{
class AssetManager2;
class TaskExecutor;

class AssetBuildTaskBase : public Task
{
protected:
    AssetManager2 &mManager;
    TaskExecutor &mExecutor;
    AssetHashId mId = 0;
    AssetRecord *mRecord = nullptr;
    std::promise<void> mPromise;

    void update_asset_status(AssetStatus status);

public:
    AssetBuildTaskBase(
        AssetManager2 &manager,
        TaskExecutor &executor,
        AssetHashId id,
        AssetRecord *record);

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
        const AssetHashId id,
        AssetRecord *record,
        std::unique_ptr<BuilderT> builder)
        : AssetBuildTaskBase(manager, executor, id, record)
        , mBuilder(std::move(builder))
    {
    }

    void run() override
    {
        // todo: hash dependency content?
        mRecord->asset = mBuilder->construct_with(
            AssetRequestProxy(&mManager, &mExecutor, mId)
        );
    }
};
}
