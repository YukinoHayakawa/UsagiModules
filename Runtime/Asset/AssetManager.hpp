#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include <Usagi/Runtime/TaskExecutor.hpp>

#include "Asset.hpp"
#include "Package/AssetPackage.hpp"

namespace usagi
{
class SecondaryAssetHandlerBase;

class AssetManager
{
    std::mutex mPackageMutex;
    std::vector<std::shared_ptr<AssetPackage>> mPackages;

    bool create_query(
        std::string_view path,
        StackPolymorphicObject<AssetQuery> &query);

    std::mutex mPrimaryAssetTableMutex;
    std::map<std::string, PrimaryAssetMeta, std::less<>> mPrimaryAssets;
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

    std::mutex mSecondaryAssetTableMutex;

    struct SecondaryAssetAuxInfo
    {
        // todo perf
        mutable std::vector<std::optional<PrimaryAssetRef>>
            primary_dependencies;
        mutable std::unique_ptr<SecondaryAssetHandlerBase> handler;

        struct PseudoMeta {
            mutable std::unique_ptr<SecondaryAsset> secondary;
            AssetCacheSignature signature;
            // AssetPackage *package = nullptr;
            mutable AssetStatus status : 8 = AssetStatus::SECONDARY_MISSING;
            mutable std::uint64_t loading_task_id : 56 = -1;

            ~PseudoMeta();

            operator SecondaryAssetMeta() const
            {
                // Slice the state directly as it is isomorphic with the
                // return type.
                // (unique_ptr is zero-cost abstraction over raw ptr)
                return reinterpret_cast<const SecondaryAssetMeta &>(*this);
            }
        } meta;

        static_assert(sizeof(PseudoMeta) == sizeof(SecondaryAssetMeta));

        SecondaryAssetAuxInfo(
            AssetCacheSignature sig,
            std::unique_ptr<SecondaryAssetHandlerBase> constructor)
            : handler(std::move(constructor))
        {
            meta.signature = sig;
            // secondary.package = primary_ref->second.package;
        }

        friend bool operator<(
            const SecondaryAssetAuxInfo &lhs,
            const SecondaryAssetAuxInfo &rhs)
        {
            return lhs.meta.signature < rhs.meta.signature;
        }

        friend bool operator<(
            const SecondaryAssetAuxInfo &lhs,
            const AssetCacheSignature &rhs)
        {
            return lhs.meta.signature < rhs;
        }

        friend bool operator<(
            const AssetCacheSignature &lhs,
            const SecondaryAssetAuxInfo &rhs)
        {
            return lhs < rhs.meta.signature;
        }
    };

    std::set<SecondaryAssetAuxInfo, std::less<>> mLoadedSecondaryAssets;
    using SecondaryAssetRef = decltype(mLoadedSecondaryAssets)::iterator;

    friend class SecondaryAssetLoadingTask;

public:
    void add_package(std::shared_ptr<AssetPackage> package);

    // When work queue set to nullptr, the asset will not be loaded.
    // The state of primary asset is copy-returned to prevent the returned
    // value being volatile since it may be modified in other threads by the
    // background loading job.
    PrimaryAssetMeta primary_asset(
        std::string_view asset_path,
        TaskExecutor *work_queue = nullptr);

    // Query the secondary asset cache. This won't cause any loading of
    // asset if the asset was not found in the cache. To load the asset,
    // a asset constructor must be provided via the other overload. A work
    // queue is also required for actually issuing the task in background.
    // Otherwise, only the signature of the secondary asset will be calculated
    // from the construction configuration, provided by the constructor.
    SecondaryAssetMeta secondary_asset(
        AssetCacheSignature signature);

    // Construct a secondary asset using the provided handler.
    SecondaryAssetMeta secondary_asset(
        std::unique_ptr<SecondaryAssetHandlerBase> handler,
        TaskExecutor &work_queue);
};
}
