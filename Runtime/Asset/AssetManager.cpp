﻿#include "AssetManager.hpp"

#include <thread>

#include "External/xxhash/xxhash64.h"

#include <Usagi/Library/Memory/StackPolymorphicObject.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Runtime/Task.hpp>

#include "AssetQuery.hpp"
#include "SecondaryAsset.hpp"
#include "SecondaryAssetHandler.hpp"

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
        , mEntry(entry)
    {
    }

    bool precondition() override
    {
        return mEntry->second.status == AssetStatus::QUEUED;
    }

    void on_started() override
    {
        mEntry->second.status = AssetStatus::LOADING;
    }

    void run() override
    {
        StackPolymorphicObject<AssetQuery> q;
        mPackage->create_query(mAssetPath, q);
        q->fetch();
        assert(q->prefetched());
        mEntry->second.region = q->memory_region();
        mEntry->second.fingerprint = q->fingerprint();
        assert(mEntry->second.fingerprint != 0);
    }

    void on_finished() override
    {
        mEntry->second.status = AssetStatus::READY;
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
        const auto a = mEntry->meta.status ==
            AssetStatus::QUEUED;
        const auto b = mEntry->handler != nullptr;
        for(auto &&d : mEntry->primary_dependencies)
            // check primary dependency status if the dependency is set
            if(d.has_value() &&
                d.value()->second.status != AssetStatus::READY)
                return false;
        return a && b;
    }

    void on_started() override
    {
        mEntry->meta.status = AssetStatus::LOADING;
    }

    void run() override
    {
        XXHash64 hasher(0);

        // fetch primary dependencies & build the dependency fingerprint
        // todo meme & perf
        std::vector<std::optional<PrimaryAssetMeta>> meta;
        meta.reserve(mEntry->primary_dependencies.size());
        std::ranges::transform(
            mEntry->primary_dependencies,
            std::back_inserter(meta),
            [&](auto &&d) -> std::optional<PrimaryAssetMeta> {
                if(d.has_value())
                {
                    // todo should the order be significant? (probably not assuming the SAH has a stable behavior)
                    assert(d.value()->second.fingerprint != 0);
                    hasher.add(
                        &d.value()->second.fingerprint,
                        sizeof(decltype(d.value()->second.fingerprint))
                    );
                    return d.value()->second;

                }
                return { };
            }
        );

        // build the secondary asset
        auto object = mEntry->handler->construct(meta);
        mEntry->meta.secondary = std::move(object);
        mEntry->meta.fingerprint_dep_content = hasher.hash();
    }

    void on_finished() override
    {
        if(mEntry->meta.secondary)
            mEntry->meta.status = AssetStatus::READY;
        USAGI_THROW(std::runtime_error("Asset failed to load."));
    }

    bool postcondition() override
    {
        return mEntry->meta.status == AssetStatus::READY;
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

AssetManager::SecondaryAssetAuxInfo::PseudoMeta::~PseudoMeta() = default;

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
        assert((it->second.status == AssetStatus::EXIST ||
            it->second.status == AssetStatus::QUEUED ||
            it->second.status == AssetStatus::LOADING ||
            it->second.status == AssetStatus::READY) &&
            "Asset not in valid state."
        );
        // If either the client doesn't want to load the asset, or the asset
        // is already being loaded, return its state directly.
        if(!load || it->second.status != AssetStatus::EXIST)
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

        const PrimaryAssetMeta asset
        {
            .package = query->package(),
            .status = AssetStatus::EXIST
        };

        bool inserted;
        std::tie(it, inserted) = mPrimaryAssets.try_emplace(
            std::string(asset_path), asset);

        assert(inserted);

        // The client only wants to query the existence of the asset.
        if(!load)
        {
            assert(it->second.status == AssetStatus::EXIST);
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
    it->second.status = AssetStatus::QUEUED;

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

PrimaryAssetMeta AssetManager::primary_asset(
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

SecondaryAssetMeta AssetManager::secondary_asset(
    const AssetFingerprint signature)
{
    LockGuard lk(mSecondaryAssetTableMutex);

    const auto it = mLoadedSecondaryAssets.find(signature);
    if(it == mLoadedSecondaryAssets.end())
        return { };

    // The asset exists in cache. Return is regardless of its state.
    return it->meta;
}

SecondaryAssetMeta AssetManager::secondary_asset(
    std::unique_ptr<SecondaryAssetHandlerBase> handler,
    TaskExecutor &work_queue)
{
    assert(handler && "No secondary asset handler is provided.");

    struct Xxh64Hasher : SecondaryAssetHandlerBase::Hasher
    {
        XXHash64 hasher { 0 };

        void append(const void *data, const std::size_t size) override
        {
            hasher.add(data, size);
        }
    } hasher;

    const auto handler_ptr = handler.get();
    const auto type_hash = typeid(handler_ptr).hash_code();
    hasher.append(&type_hash, sizeof(type_hash));
    handler->append_build_parameters(hasher);

    // Include asset ref paths in the fingerprint
    for(std::size_t i = 0;; ++i)
    {
        const auto dep = handler->primary_dependencies(i);
        if(!dep.has_value()) break;
        if(!dep->empty())
            hasher.append(dep->data(), dep->size());
    }

    const auto fingerprint_build = hasher.hasher.hash();

    LockGuard slk(mSecondaryAssetTableMutex);

    auto it_sec = mLoadedSecondaryAssets.find(fingerprint_build);
    // If an entry exists for the asset, it must be either being constructed
    // or already constructed. Just return the current state of the entry.
    // Avoid this kind of query considering its performance burden of creating
    // the handler.
    if(it_sec != mLoadedSecondaryAssets.end())
        return it_sec->meta;

    // Now it's sure that we are going to load the asset, ensure that an
    // entry is present. Only newly created entry goes the following path.
    std::tie(it_sec, std::ignore) = mLoadedSecondaryAssets.emplace(
        fingerprint_build,
        std::move(handler)
    );
    // Set the queued status here so if further requests arise before the task
    // is really submitted the user knows not to unnecessarily create more
    // asset handlers.
    it_sec->meta.status = AssetStatus::QUEUED;

    // We are done with the table here. Future modifications to the entry
    // must be from the loading task created later.
    slk.unlock();

    // If no cache entry was found for the requested secondary asset, query the
    // status of dependent primary assets. If any loading task is generated, we
    // will be the unique owner of it here. An entry for the primary will be
    // inserted into the table and no further loading job will be created. So if
    // we want to load the asset, this task must not be discarded and be
    // executed before the task of loading the secondary asset.

    // todo perf
    std::vector<std::uint64_t> wait_on;
    // Collect primary dependencies.
    for(std::size_t i = 0;; ++i)
    {
        const auto dep = handler_ptr->primary_dependencies(i);

        // no further dependencies
        if(!dep.has_value()) break;

        // optional dependency set empty
        if(dep->empty())
        {
            it_sec->primary_dependencies.emplace_back();
            continue;
        }

        LockGuard plk(mPrimaryAssetTableMutex);

        // Query the status of corresponding primary asset. If no loading task
        // is returned, it means an task must be already active. Otherwise, the
        // only opportunity of submitting that task lies below, because only one
        // loading task will ever be created for each primary asset. So this
        // critical section doesn't have to last to the point when this task is
        // submitted. however, secondary table is still locked throughout the
        // whole process.
        auto [primary, it_pri, task_pri] = query_primary_asset_nolock(
            dep.value(),
            true
        );

        plk.unlock();

        // bug: even if the asset is missing a dependency, the loading tasks for other dependencies will still be created.
        if(primary.status == AssetStatus::MISSING)
        {
            it_sec->meta.status = AssetStatus::MISSING_DEPENDENCY;
        }
        else
        {
            if(task_pri)
                it_pri->second.loading_task_id = work_queue.submit(
                    std::move(task_pri));
            else
                assert(primary.status == AssetStatus::READY &&
                    "The primary asset was not loaded, but no loading task "
                    "was created.");

            assert(primary.loading_task_id != TaskExecutor::INVALID_TASK &&
                "The loading task id for the primary asset has been lost.");

            it_sec->primary_dependencies.emplace_back(it_pri);
            wait_on.push_back(it_pri->second.loading_task_id);
        }
    }

    if(it_sec->meta.status != AssetStatus::MISSING_DEPENDENCY)
    {
        auto task_sec = std::make_unique<SecondaryAssetLoadingTask>(it_sec);

        it_sec->meta.loading_task_id = work_queue.submit(
            std::move(task_sec),
            std::move(wait_on)
        );
    }

    // Note that asset status change is possible here. But it doesn't hurt as
    // status consistency is maintained in the loading task. It will only
    // be ready when all the processing is done.
    return it_sec->meta;
}
}
