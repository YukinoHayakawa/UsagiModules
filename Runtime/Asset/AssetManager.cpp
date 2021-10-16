#include "AssetManager.hpp"

#include <thread>

#include <Usagi/Library/Memory/StackPolymorphicObject.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>

#include "AssetQuery.hpp"

namespace usagi
{
bool AssetManager::PrimaryAssetLoadingTask::precondition()
{
    return mEntry->second.status == AssetStatus::PRIMARY_PENDING;
}

void AssetManager::PrimaryAssetLoadingTask::on_started()
{
    mEntry->second.status = AssetStatus::PRIMARY_LOADING;
}

void AssetManager::PrimaryAssetLoadingTask::run()
{
    StackPolymorphicObject<AssetQuery> q;
    mPackage->create_query(mAssetPath, q);
    q->fetch();
    assert(q->prefetched());
}

void AssetManager::PrimaryAssetLoadingTask::on_finished()
{
    mEntry->second.status = AssetStatus::PRIMARY_READY;
}

bool AssetManager::create_query(
    const std::string_view path,
    StackPolymorphicObject<AssetQuery> &query)
{
    std::lock_guard lock(mPackageMutex);

    // search in reverse order of added packages, so that packages added later
    // override those added earlier.
    for(auto it = mPackages.rbegin(); it != mPackages.rend(); ++it)
        if((*it)->create_query(path, query))
            return true;

    return false;
}

void AssetManager::add_package(std::shared_ptr<AssetPackage> package)
{
    std::lock_guard lock(mPackageMutex);

    mPackages.emplace_back(std::move(package));
}

PrimaryAsset AssetManager::primary_asset(
    std::string_view asset_path,
    TaskExecutor *work_queue)
{
    const bool load = work_queue != nullptr;

    // Find the table entry first. If an entry was found, whether it's loaded
    // or not, return it. So that no two racing loading job will be created.
    LockGuard lock(mPrimaryAssetTableMutex);

    auto it = mLoadedPrimaryAssets.find(asset_path);

    // Either it's being loaded or already loaded, return its status.
    // todo: update table when packages change.
    if(it != mLoadedPrimaryAssets.end())
    {
        assert((it->second.status == AssetStatus::PRIMARY_FOUND ||
            it->second.status == AssetStatus::PRIMARY_PENDING ||
            it->second.status == AssetStatus::PRIMARY_LOADING ||
            it->second.status == AssetStatus::PRIMARY_READY) &&
            "Asset not in valid state."
        );
        // If either the client doesn't want to load the asset, or the asset
        // is already being loaded, return its state directly.
        if(!load || it->second.status != AssetStatus::PRIMARY_FOUND)
            return it->second;
    }

    StackPolymorphicObject<AssetQuery> query;

    // If the asset is not in the table, we have to find the asset from
    // packages.
    if(it == mLoadedPrimaryAssets.end())
    {
        if(!create_query(asset_path, query))
            // asset not found
            return { };

        const PrimaryAsset asset
        {
            .package = query->package(),
            .status = AssetStatus::PRIMARY_FOUND
        };

        bool inserted;
        std::tie(it, inserted) = mLoadedPrimaryAssets.try_emplace(
            std::string(asset_path), asset);

        assert(inserted);

        // The client only wants to query the existence of the asset.
        if(!load)
        {
            assert(it->second.status == AssetStatus::PRIMARY_FOUND);
            return it->second;
        }
    }
    // The asset is already in the table, but not loaded. We have to create
    // the task.
    else
    {
        it->second.package->create_query(asset_path, query);
    }

    assert(load);
    it->second.status = AssetStatus::PRIMARY_PENDING;
    const auto last_status = it->second;

    lock.unlock();

    // The three paths where the asset will not be loaded are dealt with.
    // Now create the task of loading the asset.

    auto task = std::make_unique<PrimaryAssetLoadingTask>(
        std::string(asset_path),
        query->package()->shared_from_this(),
        it
    );

    work_queue->submit(std::move(task));

    return last_status;
}
}
