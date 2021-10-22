#include "AssetManager.hpp"

#include <thread>

#include <Usagi/Library/Memory/StackPolymorphicObject.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Runtime/Task.hpp>

#include "AssetQuery.hpp"
#include "SecondaryAssetConstructor.hpp"

namespace usagi
{
class PrimaryAssetLoadingTask : public Task
{
    std::string mAssetPath;
    std::shared_ptr<AssetPackage> mPackage;
    AssetManager::PrimaryAssetRef mEntry;

public:
    PrimaryAssetLoadingTask(
        std::string asset_path,
        std::shared_ptr<AssetPackage> package,
        AssetManager::PrimaryAssetRef entry)
        : mAssetPath(std::move(asset_path))
        , mPackage(std::move(package))
        , mEntry(std::move(entry))
    {
    }

    bool precondition() override
    {
        return mEntry->second.status == AssetStatus::PRIMARY_PENDING;
    }

    void on_started() override
    {
        mEntry->second.status = AssetStatus::PRIMARY_LOADING;
    }

    void run() override
    {
        StackPolymorphicObject<AssetQuery> q;
        mPackage->create_query(mAssetPath, q);
        q->fetch();
        assert(q->prefetched());
    }

    void on_finished() override
    {
        mEntry->second.status = AssetStatus::PRIMARY_READY;
    }
};

class SecondaryAssetLoadingTask : public Task
{
    AssetManager::SecondaryAssetRef mEntry;

public:
    SecondaryAssetLoadingTask(AssetManager::SecondaryAssetRef entry)
        : mEntry(entry)
    {
    }

    bool precondition() override
    {
        const auto a = mEntry->primary_ref->second.status ==
            AssetStatus::PRIMARY_READY;
        const auto b = mEntry->secondary.status ==
            AssetStatus::SECONDARY_PENDING;
        const auto c = mEntry->constructor != nullptr;

        return a && b && c;
    }

    void on_started() override
    {
        mEntry->secondary.status = AssetStatus::SECONDARY_PROCESSING;
    }

    void run() override
    {
        // feed the raw memory of primary asset to secondary asset constructor
        const auto raw = mEntry->primary_ref->second.region;
        auto object = mEntry->constructor->construct(raw);
        mEntry->secondary.object = std::move(object);
    }

    void on_finished() override
    {
        mEntry->secondary.status = AssetStatus::SECONDARY_READY;
    }
};

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

AssetManager::PrimaryAssetQueryResult AssetManager::query_primary_asset_nolock(
    const std::string_view asset_path,
    const bool load)
{
    // Find the table entry first. If an entry was found, whether it's loaded
    // or not, return it. So that no two racing loading job will be created.

    // todo: a same asset may be requested using different paths
    // (e.g. /a/b/c and /a/d/../b/c). though this is kinda ridiculous but it
    // can arise during game development especially from the artist side. the
    // path should be somehow normalized to prevent loading an asset multiple
    // times. (though some package impl may have internal mechanism of
    // preventing this, e.g. filesystem package can query the canonical path
    // of a file and use it as a unique identifier)
    auto it = mPrimaryAssets.find(asset_path);

    // Either it's being loaded or already loaded, return its status.
    // todo: update table when packages change.
    if(it != mPrimaryAssets.end())
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
            return { it->second, it, { } };
    }

    StackPolymorphicObject<AssetQuery> query;

    // If the asset is not in the table, we have to find the asset from
    // packages.
    if(it == mPrimaryAssets.end())
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
        std::tie(it, inserted) = mPrimaryAssets.try_emplace(
            std::string(asset_path), asset);

        assert(inserted);

        // The client only wants to query the existence of the asset.
        if(!load)
        {
            assert(it->second.status == AssetStatus::PRIMARY_FOUND);
            return { it->second, it, { } };
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

    // lock.unlock();

    // The three paths where the asset will not be loaded are dealt with.
    // Now create the task of loading the asset.

    auto task = std::make_unique<PrimaryAssetLoadingTask>(
        std::string(asset_path),
        query->package()->shared_from_this(),
        it
    );

    return { it->second, it, std::move(task) };
}

PrimaryAsset AssetManager::primary_asset(
    const std::string_view asset_path,
    TaskExecutor *work_queue)
{
    LockGuard lock(mPrimaryAssetTableMutex);

    const bool load = work_queue != nullptr;
    auto [primary, it, task] = query_primary_asset_nolock(asset_path, load);

    if(task)
    {
        const auto task_id = work_queue->submit(std::move(task));
        // assignment of task id must be lock-protected. because a secondary
        // asset loading request may find that a primary asset is in a loading
        // status, but couldn't fetch a valid task id just before this
        // assignment.
        // this id is used for tracing the status of the task, and building
        // task dependencies when loading secondary assets.
        it->second.loading_task_id = task_id;
    }

    return primary;
}

SecondaryAsset AssetManager::secondary_asset(
    const AssetCacheSignature signature)
{
    LockGuard lk(mSecondaryAssetTableMutex);

    const auto it = mLoadedSecondaryAssets.find(signature);
    if(it == mLoadedSecondaryAssets.end())
        return { };

    // The asset exists in cache. Return is regardless of its state.
    return it->secondary;
}

SecondaryAsset AssetManager::secondary_asset(
    std::string_view primary_asset_path,
    std::unique_ptr<SecondaryAssetConstructor> constructor,
    TaskExecutor *work_queue)
{
    const bool load = work_queue != nullptr;
    const auto sig = constructor->signature();

    LockGuard slk(mSecondaryAssetTableMutex);

    auto it_sec = mLoadedSecondaryAssets.find(sig);
    // If an entry exists for the asset, it must be either being constructed
    // or already constructed. Just return the current state of the entry.
    // Avoid this kind of query considering its performance burden.
    if(it_sec != mLoadedSecondaryAssets.end())
    {
        auto sec = it_sec->secondary;
        // todo pending becomes a useless state?
        if(it_sec->secondary.status <= AssetStatus::SECONDARY_PENDING)
            // don't assign to the original state as it may cause race condition
            sec.status = it_sec->primary_ref->second.status;
        return sec;
    }

    // If no cache entry was found for the requested secondary asset, query
    // the status of the corresponding primary asset.
    // If the caller wants to load the asset, first check whether the primary
    // asset is loaded. If any loading task is generated, we will be the
    // unique owner of it here. An entry for the primary will be inserted into
    // the table and no further loading job will be created. So if we want
    // to load the asset, this task must not be discarded and be executed
    // before the job of loading the secondary asset.

    LockGuard plk(mPrimaryAssetTableMutex);

    // Query the status of corresponding primary asset. If no loading task is
    // returned, it means an task must be already active. Otherwise, the only
    // opportunity of submitting that task lies below, because only one loading
    // task will ever be created for each primary asset. So this critical
    // section doesn't have to last to the point when this task is submitted.
    // however, secondary table is still locked throughout the whole process.
    auto [primary, it_pri, task_pri] = query_primary_asset_nolock(
        primary_asset_path, load);

    plk.unlock();

    // If the caller doesn't want to load the asset, return the status of
    // the primary asset is returned instead.
    // The signature is not very useful here as to actually load the asset
    // this overload of method must be called again.
    if(!load)
    {
        assert(!task_pri && "Primary asset query produced a loading task "
            "when the caller doesn't want to load the asset.");
        SecondaryAsset sec;
        sec.signature = sig;
        sec.package = primary.package;
        sec.status = primary.status;
        return sec;
    }

    // Now it's sure that we are going to load the asset, ensure that an
    // entry is present. Only newly created entry goes the following path.
    std::tie(it_sec, std::ignore) = mLoadedSecondaryAssets.emplace(sig);

    // Update the entry with the status of the primary asset.
    // The actual key (sec.signature) not modified here so it's safe
    // to do the const_cast.
    auto &sec = const_cast<SecondaryAsset &>(it_sec->secondary);
    sec.package = primary.package;
    sec.status = primary.status;

    // We are done with the table here. Future modifications to the entry
    // must be from the loading task created later.
    slk.unlock();

    if(task_pri)
    {
        it_pri->second.loading_task_id = work_queue->submit(
            std::move(task_pri));
    }
    else
    {
        assert(primary.status == AssetStatus::PRIMARY_READY &&
            "The primary asset was not loaded, but no loading task "
            "has been created.");
    }

    assert(primary.loading_task_id != TaskExecutor::INVALID_TASK &&
        "The loading task id for the primary asset has been lost.");

    // Loading task for secondary asset won't be created if an entry already
    // exists, so no race condition of changing asset status here.
    sec.status = AssetStatus::SECONDARY_PENDING;

    auto task_sec = std::make_unique<SecondaryAssetLoadingTask>(it_sec);
    sec.loading_task_id = work_queue->submit(
        std::move(task_sec),
        primary.status == AssetStatus::PRIMARY_READY
            ? TaskExecutor::INVALID_TASK
            : it_pri->second.loading_task_id
    );

    // Note that asset status change is possible here. But it doesn't hurt as
    // status consistency is maintained in the loading task. It will only
    // be ready when all the processing is done.
    return sec;
}
}
