#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <Usagi/Runtime/Task.hpp>
#include <Usagi/Runtime/TaskExecutor.hpp>

#include "Asset.hpp"
#include "Package/AssetPackage.hpp"

namespace usagi
{
class AssetManager
{
    std::mutex mPrimaryAssetTableMutex;
    std::map<std::string, PrimaryAsset, std::less<>> mLoadedPrimaryAssets;
    using PrimaryAssetEntry = decltype(mLoadedPrimaryAssets)::iterator;

    class PrimaryAssetLoadingTask : public Task
    {
        std::string mAssetPath;
        std::shared_ptr<AssetPackage> mPackage;
        PrimaryAssetEntry mEntry;

    public:
        PrimaryAssetLoadingTask(
            std::string asset_path,
            std::shared_ptr<AssetPackage> package,
            PrimaryAssetEntry entry)
            : mAssetPath(std::move(asset_path))
            , mPackage(std::move(package))
            , mEntry(std::move(entry))
        {
        }

        bool precondition() override;
        void on_started() override;
        void run() override;
        void on_finished() override;
    };

    friend class PrimaryAssetLoadingTask;

    std::mutex mPackageMutex;
    std::vector<std::shared_ptr<AssetPackage>> mPackages;

    bool create_query(
        std::string_view path,
        StackPolymorphicObject<AssetQuery> &query);

public:
    void add_package(std::shared_ptr<AssetPackage> package);

    PrimaryAsset primary_asset(
        std::string_view asset_path,
        // When set to nullptr, the asset will not be loaded.
        TaskExecutor *work_queue = nullptr);
};
}
