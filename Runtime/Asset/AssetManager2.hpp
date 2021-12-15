#pragma once

#include <map>
#include <shared_mutex>

#include <Usagi/Runtime/ErrorHandling.hpp>

#include "AssetHasher.hpp"
#include "details/AssetBuilder.hpp"
#include "details/AssetPackageManager.hpp"
#include "details/AssetRecord.hpp"
#include "details/AssetAccessProxy.hpp"

namespace usagi
{
class AssetBuildTask;
class AssetBuilder;

class AssetManager2
{
    AssetPackageManager mPackageManager;
    std::shared_mutex mPackageMutex;

    std::map<AssetHashId, AssetRecord> mAssetRecords;
    std::shared_mutex mAssetTableMutex;
public:
    using AssetRecordRef = decltype(mAssetRecords)::iterator;

private:
    // Extracted from the main asset request handling function to reduce code
    // size.
    std::pair<AssetRecordRef, bool> try_emplace(AssetHashId id);

    void make_submit_build_task(
        TaskExecutor &executor,
        std::unique_ptr<AssetBuilder> builder,
        AssetRecordRef it);

    template <typename Builder, typename... Args>
    AssetRecordRef load_asset(TaskExecutor &executor, Args &&...args)
        requires IsProperAssetBuilder<Builder>
    {
        // Use the type hash of the builder and build parameters to build a
        // unique id of the requested asset.
        AssetHasher hasher;
        // The hash code of the builder type can be used to detect collision.
        const auto type_hash = typeid(Builder).hash_code();
        hasher.append(type_hash);
        (..., hasher.append(args));

        // Use the asset hash to query the asset entry table.
        // Note: critical section inside.
        const auto [it, inserted] = try_emplace(hasher.hash());

        /*
        * If the asset is not loaded, construct the builder and delegate the
        * construction task to the task executor provided by the user.
        * Then return a notice telling the user of the current state of the
        * asset. Note that the state could be volatile due to the async
        * construction of the asset.
        * Only one of the concurrent requests will be able to insert the
        * asset. So the build task will also be uniquely created.
        * todo: what if the asset is being evicted here?
        */
        if(inserted == true)
        {
            /*
            * Builder is responsible for constructing an asset. Build task is
            * a wrapper that holds the builder. The build task is submitted
            * to the executor containing the builder and when it is executed,
            * it will invoke the builder with this asset manager and the
            * task executor provided by the user. The builder can request
            * other assets via the provided asset manager. The requests will
            * be recorded and regarded as a dependency relationship that will
            * be visited when the depended assets are changed. In that case,
            * the outdated assets will be unloaded.
            */
            auto builder = std::make_unique<Builder>(
                std::forward<Args>(args)...
            );
            make_submit_build_task(executor, std::move(builder), it);
        }
        else
        {
            USAGI_THROW(std::runtime_error("Hash collision."));
        }

        return it;
    }

    /*
     * Grab a snapshot of the asset state. The build task always assigns
     * asset status prior to the asset pointer, so it's guaranteed that
     * a valid asset will be returned when the status indicates so.
     */
    static AssetAccessProxy result_from(AssetRecordRef it);

public:
    // ========================= Package Management ========================= //

    // todo: adding/removing an asset package should not happen while there is any active asset loading task, or during any request of assets. because loading tasks may hold references to asset entries and changes in packages may invalidate them.
    void add_package(std::unique_ptr<AssetPackage> package);
    void remove_package(AssetPackage *package);

    AssetQuery * create_asset_query(AssetPath path, MemoryArena &arena);

    // =========================== Asset Request ============================ //

    /*
     * Request an asset. The builder type and args will be used to construct
     * an unique id for identifying the asset. Therefore, the arg types must
     * have certain hash helper function implemented, which will be found via
     * ADL and participate in the building of the id. The id is guaranteed to
     * be unique for each unique set of build parameters.
     */
    template <typename Builder, typename... Args>
    AssetAccessProxy asset(TaskExecutor &executor, Args &&...args)
    {
        return result_from(
            load_asset<Builder>(executor, std::forward<Args>(args)...)
        );
    }

    /*
     * After having the asset id, it can be used to access the asset with
     * better performance (without having to hash the parameters). However,
     * if the asset couldn't be found, the previous function have to be called
     * again.
     */
    AssetAccessProxy asset(AssetHashId id);
};
}
