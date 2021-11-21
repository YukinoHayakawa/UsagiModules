#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <variant>
#include <deque>
#include <future>

#include <Usagi/Runtime/TaskExecutor.hpp>

#include "Asset.hpp"
#include "AssetDependencyGraph.hpp"
#include "Package/AssetPackage.hpp"

namespace usagi
{
class SecondaryAssetHandlerBase;

// todo: remove finished tasks from work queue
class AssetManager
{
    // ============================== Packages ============================== //

    struct PackageEntry
    {
        std::uint64_t vertex = -1;
        std::unique_ptr<AssetPackage> package;
    };

    std::deque<PackageEntry> mPackages;
    using PackageRef = decltype(mPackages)::iterator;

    bool create_query(
        std::string_view path,
        StackPolymorphicObject<AssetQuery> &query);

    // =========================== Primary Assets =========================== //

    struct PrimaryAssetEntry : PrimaryAssetMeta
    {
        std::uint64_t loading_task_id = -1;
        std::uint64_t vertex_idx = -1;
        std::shared_future<PrimaryAssetMeta> future;

        PrimaryAssetMeta meta() const
        {
            return static_cast<PrimaryAssetMeta>(*this);
        }
    };

    std::mutex mPrimaryAssetTableMutex;
    std::map<std::string, PrimaryAssetEntry, std::less<>> mPrimaryAssets;
    using PrimaryAssetRef = decltype(mPrimaryAssets)::iterator;

    friend class PrimaryAssetLoadingTask;

    struct PrimaryAssetQueryResult
    {
        PrimaryAssetMeta primary_snapshot;
        PrimaryAssetRef primary_ref;
        std::unique_ptr<PrimaryAssetLoadingTask> loading_task;
    };

    PrimaryAssetQueryResult query_primary_asset_nolock(
        std::string_view asset_path,
        bool load);

    PrimaryAssetQueryResult ensure_primary_asset_entry(
        std::string_view asset_path,
        TaskExecutor *work_queue = nullptr);

    // ========================== Secondary Assets ========================== //

    std::mutex mSecondaryAssetTableMutex;

    struct SecondaryAssetEntry
    {
        std::unique_ptr<SecondaryAsset> asset;
        // AssetFingerprint fingerprint_build = 0;
        AssetFingerprint fingerprint_dep_content = 0;
        AssetStatus status = AssetStatus::MISSING;

        std::uint64_t loading_task_id = -1;
        std::uint64_t vertex_idx = -1;
        std::unique_ptr<SecondaryAssetHandlerBase> handler;
        std::shared_future<SecondaryAssetMeta> future;

        SecondaryAssetEntry() = default;
        SecondaryAssetEntry(SecondaryAssetEntry &&other) noexcept = default;
        SecondaryAssetEntry & operator=(
            SecondaryAssetEntry &&other) noexcept = default;
        ~SecondaryAssetEntry();
    };

    std::map<AssetFingerprint, SecondaryAssetEntry> mSecondaryAssets;
    using SecondaryAssetRef = decltype(mSecondaryAssets)::iterator;

    static SecondaryAssetMeta meta_from_entry(SecondaryAssetRef entry);

    SecondaryAssetRef ensure_secondary_asset_entry(
        std::unique_ptr<SecondaryAssetHandlerBase> handler,
        TaskExecutor &work_queue);

    // ========================== Dependency Graph ========================== //

    std::mutex mDependencyMutex;

    friend class SecondaryAssetLoadingTask;

    using VertexT = std::variant<
        PackageRef,
        PrimaryAssetRef,
        SecondaryAssetRef
    >;

    // Whenever there is any change that can invalidate an asset, or would
    // resolve an asset to another package, it's necessary to remove the
    // affected assets to reflect that change. Specifically, when a package is
    // added, all loaded assets should be checked with the new package to see
    // if any asset is overridden by it and remove the corresponding assets
    // from the loaded asset cache. All secondary assets built with the removed
    // primary assets should also be recursively removed from the cache, and
    // they will be rebuilt upon the next request of access.
    AssetDependencyGraph<VertexT> mDependencyGraph;

    PrimaryAssetRef invalidate_primary_asset(PrimaryAssetRef asset);
    void invalidate_secondary_asset(SecondaryAssetRef asset);
    void invalidate_secondary_asset_helper(std::uint64_t start_v);

public:
    // todo: adding/removing an asset package should not happen while there is any active asset loading task, or during any request of assets. because loading tasks may hold references to asset entries and changes in packages may invalidate them.
    void add_package(std::unique_ptr<AssetPackage> package);
    void remove_package(AssetPackage *package);

    // When work queue set to nullptr, the asset will not be loaded.
    // The state of primary asset is copy-returned to prevent the returned
    // value being volatile since it may be modified in other threads by the
    // background loading job.
    [[nodiscard]]
    PrimaryAssetMeta primary_asset(
        std::string_view asset_path,
        TaskExecutor *work_queue = nullptr);

    // Query the secondary asset cache. This won't cause any loading of
    // asset if the asset was not found in the cache. To load the asset,
    // a asset constructor must be provided via the other overload. A work
    // queue is also required for actually issuing the task in background.
    // Otherwise, only the signature of the secondary asset will be calculated
    // from the construction configuration, provided by the constructor.
    [[nodiscard]]
    SecondaryAssetMeta secondary_asset(
        AssetFingerprint fingerprint_build);

    // Construct a secondary asset using the provided handler.
    [[nodiscard]]
    SecondaryAssetMeta secondary_asset(
        std::unique_ptr<SecondaryAssetHandlerBase> handler,
        TaskExecutor &work_queue);

    // bug: the current concurrency model may cause deadlocks
    // https://developercommunity.visualstudio.com/t/nested-stdasync-causes-deadlock/269365

private:
    friend class SecondaryAssetHandlerBase;

    // todo: provide way of allowing the asset handler to yield after emitting async requests and notify the scheduler
    [[nodiscard]]
    std::shared_future<PrimaryAssetMeta> primary_asset_async(
        std::string_view asset_path,
        TaskExecutor &work_queue);

    [[nodiscard]]
    std::shared_future<SecondaryAssetMeta> secondary_asset_async(
        std::unique_ptr<SecondaryAssetHandlerBase> handler,
        TaskExecutor &work_queue,
        SecondaryAssetHandlerBase *calling_handler = nullptr);
};
}
