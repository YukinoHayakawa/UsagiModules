#include "AssetManager.hpp"

#include <thread>
#include <future>

#include "External/xxhash/xxhash64.h"

#include <Usagi/Library/Memory/StackPolymorphicObject.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/Task.hpp>

#include "AssetQuery.hpp"
#include "SecondaryAsset.hpp"
#include "SecondaryAssetHandler.hpp"

namespace usagi
{
class PrimaryAssetLoadingTask : public Task
{
    std::string mAssetPath;
    AssetPackage *mPackage;
    AssetManager::PrimaryAssetRef mEntry;
    std::promise<PrimaryAssetMeta> mPromise;

public:
    PrimaryAssetLoadingTask(
        std::string asset_path,
        AssetPackage *package,
        AssetManager::PrimaryAssetRef entry)
        : mAssetPath(std::move(asset_path))
        , mPackage(package)
        , mEntry(entry)
    {
    }

    std::future<PrimaryAssetMeta> future()
    {
        return mPromise.get_future();
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
        mPromise.set_value(mEntry->second);
    }

    void on_finished() override
    {
        mEntry->second.status = AssetStatus::READY;
    }
};

class SecondaryAssetLoadingTask : public Task
{
    AssetManager *mManager = nullptr;
    TaskExecutor *mExecutor = nullptr;
    AssetManager::SecondaryAssetRef mEntry;
    std::promise<SecondaryAssetMeta> mPromise;

public:
    SecondaryAssetLoadingTask(
        AssetManager *manager,
        TaskExecutor *executor,
        AssetManager::SecondaryAssetRef entry)
        : mManager(manager)
        , mExecutor(executor)
        , mEntry(std::move(entry))
    {
    }

    std::future<SecondaryAssetMeta> future()
    {
        return mPromise.get_future();
    }

    bool precondition() override
    {
        const auto a = mEntry->second.status == AssetStatus::QUEUED;
        const auto b = mEntry->second.handler != nullptr;
        return a && b;
    }

    void on_started() override
    {
        mEntry->second.status = AssetStatus::LOADING;
    }

    void run() override
    {
        // XXHash64 hasher(0);

        // mEntry->meta().fingerprint_dep_content = hasher.hash();
        // mPromise.set_value(mEntry->meta());

        // build the secondary asset
        auto object = mEntry->second.handler->construct(*mManager, *mExecutor);
        mEntry->second.asset = std::move(object);
        // mEntry->fingerprint_dep_content = hasher.hash();
    }

    void on_finished() override
    {
        if(mEntry->second.asset)
            mEntry->second.status = AssetStatus::READY;
        else
            USAGI_THROW(std::runtime_error("Asset failed to load."));

        mPromise.set_value(AssetManager::meta_from_entry(mEntry));
    }

    bool postcondition() override
    {
        return mEntry->second.status == AssetStatus::READY;
    }
};

bool AssetManager::create_query(
    const std::string_view path,
    StackPolymorphicObject<AssetQuery> &query)
{
    // Locks the dependency graph so no packages may be added or removed
    // during our operation.
    std::lock_guard lock(mDependencyMutex);

    // search in reverse order of added packages, so that packages added later
    // override those added earlier.
    for(auto it = mPackages.rbegin(); it != mPackages.rend(); ++it)
        if((*it).package->create_query(path, query))
            return true;

    return false;
}

AssetManager::SecondaryAssetEntry::~SecondaryAssetEntry() = default;

SecondaryAssetMeta AssetManager::meta_from_entry(SecondaryAssetRef entry)
{
    SecondaryAssetMeta meta;
    meta.fingerprint_build = entry->first;
    meta.status = entry->second.status;
    meta.asset = entry->second.asset.get();
    // meta.fingerprint_dep_content
    return meta;
}

void AssetManager::invalidate_secondary_asset_helper(std::uint64_t start_v)
{
    // Find outgoing edges to depending secondary assets.
    const auto out_edges = mDependencyGraph.adjacent_vertices(start_v);

    // Recursively invalidate affected secondary assets.
    for(auto &&sec_v : out_edges)
    {
        auto &sec_vtx_data = mDependencyGraph.vertex(sec_v);
        // Check consistency (see VertexT definition)
        assert(sec_vtx_data.index() == 2);
        invalidate_secondary_asset(std::get<2>(sec_vtx_data));
    }

    // Remove the vertex from the dependency graph..
    mDependencyGraph.erase_vertex(start_v);
}

AssetManager::PrimaryAssetRef AssetManager::invalidate_primary_asset(
    PrimaryAssetRef asset)
{
    const auto &meta = asset->second;

    // Check graph consistency
    const auto &pri_back_ref = mDependencyGraph.vertex(meta.vertex_idx);
    assert(pri_back_ref.index() == 1);
    assert(std::get<1>(pri_back_ref) == asset);

    invalidate_secondary_asset_helper(meta.vertex_idx);

    LOG(debug, "[Asset] Unloading asset: {}", asset->first);

    // Remove the asset from cache. Next request to the same asset will
    // trigger a reload.
    return mPrimaryAssets.erase(asset);
}

// todo generalize to bfs traversal and put in graph lib & visitor
void AssetManager::invalidate_secondary_asset(SecondaryAssetRef asset)
{
    const auto &meta = asset->second;

    // Check graph consistency
    const auto &sec_back_ref = mDependencyGraph.vertex(meta.vertex_idx);
    assert(sec_back_ref.index() == 2);
    assert(std::get<2>(sec_back_ref) == asset);

    invalidate_secondary_asset_helper(meta.vertex_idx);

    LOG(debug, "[Asset] Unloading asset: {:16x}", asset->first);

    // Remove the asset from cache.
    mSecondaryAssets.erase(asset);
}

void AssetManager::add_package(std::unique_ptr<AssetPackage> package)
{
    const auto pkg = package.get();
    auto name = pkg->name();

    // Locks all the mutexes since we may erase asset entries.
    std::lock_guard lk0 { mDependencyMutex };
    std::lock_guard lk1 { mPrimaryAssetTableMutex };
    std::lock_guard lk2 { mSecondaryAssetTableMutex };

    // Allocate a vertex on the dependency graph for the package.
    const auto idx = mDependencyGraph.add_vertex();
    auto &v = mDependencyGraph.vertex(idx);

    PackageEntry pkg_meta;
    pkg_meta.vertex = idx;
    pkg_meta.package = std::move(package);
    {
        auto iter = mPackages.emplace(mPackages.end(), std::move(pkg_meta));
        v = iter;
    }

    LOG(debug, "[Asset] Package \"{}\" added {{v={}}}", name, idx);

    // Scan over all loaded assets and invalidate any overridden assets.
    for(auto iter = mPrimaryAssets.begin(); iter != mPrimaryAssets.end();)
    {
        auto &[path, meta] = *iter;
        StackPolymorphicObject<AssetQuery> query;
        // The new package overrides a loaded primary asset.
        if(pkg->create_query(path, query))
        {
            LOG(info, "Package \"{}\" overrides asset \"{}\"", name, path);
            iter = invalidate_primary_asset(iter);
            continue;
        }
        ++iter;
    }
}

void AssetManager::remove_package(AssetPackage *package)
{
    // todo
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
            return { it->second.meta(), it, { } };
    }

    StackPolymorphicObject<AssetQuery> query;

    // If the asset is not in the table, we have to find the asset from
    // packages.
    if(it == mPrimaryAssets.end())
    {
        if(!create_query(asset_path, query))
            // asset not found
            return { };

        {
            PrimaryAssetEntry asset;
            asset.package = query->package();
            asset.status = AssetStatus::EXIST;

            bool inserted;
            std::tie(it, inserted) = mPrimaryAssets.try_emplace(
                std::string(asset_path), asset);
            assert(inserted);
        }

        // Insert dependency graph vertex.
        it->second.vertex_idx = mDependencyGraph.add_vertex();
        mDependencyGraph.vertex(it->second.vertex_idx) = it;


        // The client only wants to query the existence of the asset.
        if(!load)
        {
            assert(it->second.status == AssetStatus::EXIST);
            return { it->second.meta(), it, { } };
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

    // The three paths where the asset will not be loaded are dealt with.
    // Now create the task of loading the asset.

    auto task = std::make_unique<PrimaryAssetLoadingTask>(
        std::string(asset_path),
        query->package(),
        it
    );

    return { it->second.meta(), it, std::move(task) };
}

AssetManager::PrimaryAssetQueryResult AssetManager::ensure_primary_asset_entry(
    const std::string_view asset_path,
    TaskExecutor *work_queue)
{
    // No need to lock the dependency mutex since package operations also
    // lock this mutex.
    LockGuard lock(mPrimaryAssetTableMutex);

    const bool load = work_queue != nullptr;
    auto [primary, it, task] = query_primary_asset_nolock(asset_path, load);

    if(task)
    {
        it->second.future = task->future();
        const auto task_id = work_queue->submit(std::move(task));
        // assignment of task id must be lock-protected. because a secondary
        // asset loading request may find that a primary asset is in a loading
        // status, but couldn't fetch a valid task id just before this
        // assignment.
        // this id is used for tracing the status of the task, and building
        // task dependencies when loading secondary assets.
        it->second.loading_task_id = task_id;
    }

    return { primary, it, { } };
}

PrimaryAssetMeta AssetManager::primary_asset(
    const std::string_view asset_path,
    TaskExecutor *work_queue)
{
    auto [primary, it, _] = ensure_primary_asset_entry(asset_path, work_queue);

    return primary;
}

SecondaryAssetMeta AssetManager::secondary_asset(
    const AssetFingerprint fingerprint_build)
{
    LockGuard lk(mSecondaryAssetTableMutex);

    const auto it = mSecondaryAssets.find(fingerprint_build);
    if(it == mSecondaryAssets.end())
        return { };

    // The asset exists in cache. Return is regardless of its state.
    return meta_from_entry(it);
}

AssetManager::SecondaryAssetRef AssetManager::ensure_secondary_asset_entry(
    std::unique_ptr<SecondaryAssetHandlerBase> handler,
    TaskExecutor &work_queue)
{
    assert(handler && "No secondary asset handler was provided.");

    struct Xxh64Hasher : SecondaryAssetHandlerBase::Hasher
    {
        XXHash64 hasher { 0 };

        Hasher & append(const void *data, const std::size_t size) override
        {
            hasher.add(data, size);
            return *this;
        }
    } hasher;

    const auto handler_ptr = handler.get();
    const auto type_hash = typeid(handler_ptr).hash_code();
    hasher.append(&type_hash, sizeof(type_hash));
    handler->append_features(hasher);

    const auto fingerprint_build = hasher.hasher.hash();

    LockGuard slk(mSecondaryAssetTableMutex);

    auto it_sec = mSecondaryAssets.lower_bound(fingerprint_build);
    // If an entry exists for the asset, it must be either being constructed
    // or already constructed. Just return the current state of the entry.
    // Avoid this kind of query considering its performance burden of creating
    // the handler.
    if(it_sec != mSecondaryAssets.end() && it_sec->first == fingerprint_build)
        return it_sec;

    // Now it's sure that we are going to load the asset, ensure that an
    // entry is present. Only newly created entry goes the following path.
    it_sec = mSecondaryAssets.emplace_hint(
        it_sec, fingerprint_build, SecondaryAssetEntry());
    it_sec->second.handler = std::move(handler);

    // Add vertex on the dependency graph
    it_sec->second.vertex_idx = mDependencyGraph.add_vertex();
    mDependencyGraph.vertex(it_sec->second.vertex_idx) = it_sec;

    // Set the queued status here so if further requests arise before the task
    // is really submitted the user knows not to unnecessarily create more
    // asset handlers.
    it_sec->second.status = AssetStatus::QUEUED;

    // We are done with the table here. Future modifications to the entry
    // must be from the loading task created later.
    slk.unlock();

    auto task_sec = std::make_unique<SecondaryAssetLoadingTask>(
        this, &work_queue, it_sec);
    it_sec->second.future = task_sec->future();
    it_sec->second.loading_task_id = work_queue.submit(std::move(task_sec));

    // Note that asset status change is possible here. But it doesn't hurt as
    // status consistency is maintained in the loading task. It will only
    // be ready when all the processing is done.
    return it_sec;
}

SecondaryAssetMeta AssetManager::secondary_asset(
    std::unique_ptr<SecondaryAssetHandlerBase> handler,
    TaskExecutor &work_queue)
{
    return meta_from_entry(
        ensure_secondary_asset_entry(std::move(handler), work_queue)
    );
}

std::shared_future<PrimaryAssetMeta> AssetManager::primary_asset_async(
    std::string_view asset_path,
    TaskExecutor &work_queue)
{
    auto [meta, it, _] = ensure_primary_asset_entry(asset_path, &work_queue);
    auto future = it->second.future;
    assert(future.valid());
    return future;
}

std::shared_future<SecondaryAssetMeta> AssetManager::secondary_asset_async(
    std::unique_ptr<SecondaryAssetHandlerBase> handler,
    TaskExecutor &work_queue)
{
    auto it = ensure_secondary_asset_entry(std::move(handler), work_queue);
    auto future = it->second.future;
    assert(future.valid());
    return future;
}
}
