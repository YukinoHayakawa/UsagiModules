#pragma once

#include <map>
#include <shared_mutex>

#include <Usagi/Runtime/ErrorHandling.hpp>

#include "AssetHasher.hpp"
#include "details/AssetBuilder.hpp"
#include "details/AssetPackageManager.hpp"
#include "details/AssetRecord.hpp"
#include "details/AssetAccessProxy.hpp"
#include "details/AssetBuildTask.hpp"

namespace usagi
{
class AssetManager2
{
protected:
    AssetPackageManager mPackageManager;
    std::shared_mutex mPackageMutex;

    std::map<AssetHashId, AssetRecord> mAssetRecords;
    std::shared_mutex mAssetTableMutex;
public:
    using AssetRecordRef = decltype(mAssetRecords)::iterator;

protected:
    // Extracted from the main asset request handling function to reduce code
    // size.
    std::pair<AssetRecordRef, bool> try_emplace(AssetHashId id);

    void submit_build_task(
        TaskExecutor &executor,
        std::unique_ptr<Task> build_task,
        AssetRecordRef it);

    template <AssetBuilder Builder>
    static void validate_builder_type(const AssetRecordRef it)
    {
        USAGI_ASSERT_THROW(
            it->second.builder_type == typeid(Builder),
            std::runtime_error(
                "AssetBuilder type doesn't match with the one used to "
                "construct the asset. Hash collision or bug?"
            )
        );
    }

    template <typename AssetT>
    static AssetAccessProxy<AssetT> make_asset_access_proxy(AssetRecordRef it)
    {
        return { it->first, &it->second };
    }

    template <AssetBuilder Builder, typename... Args>
    AssetRecordRef load_asset(TaskExecutor &executor, Args &&...args)
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
            auto task = std::make_unique<AssetBuildTask<Builder>>(
                *this,
                executor,
                it->second.asset,
                it->second.status,
                std::move(builder)
            );
            it->second.future = task->future();
            it->second.builder_type = typeid(Builder);

            submit_build_task(
                executor,
                std::unique_ptr<Task>(task.release()),
                it
            );
        }
        else
        {
            // todo: warn inefficient access to asset
            validate_builder_type<Builder>(it);
        }

        return it;
    }

    AssetRecordRef find_asset(AssetHashId id);

    friend class AbRawMemoryView;
    AssetQuery * create_asset_query(AssetPath path, MemoryArena &arena);

public:
    // ========================= Package Management ========================= //

    // todo: adding/removing an asset package should not happen while there is any active asset loading task, or during any request of assets. because loading tasks may hold references to asset entries and changes in packages may invalidate them.
    void add_package(std::unique_ptr<AssetPackage> package);
    void remove_package(AssetPackage *package);

    // =========================== Asset Request ============================ //

    /*
     * Request an asset. The builder type and args will be used to construct
     * an unique id for identifying the asset. Therefore, the arg types must
     * have certain hash helper function implemented, which will be found via
     * ADL and participate in the building of the id. The id is guaranteed to
     * be unique for each unique set of build parameters.
     * Returns a snapshot of the asset state. The build task always assigns
     * asset status prior to the asset pointer, so it's guaranteed that
     * a valid asset will be returned when the status indicates so.
     */
    template <AssetBuilder Builder, typename... Args>
    AssetAccessProxy<typename AssetBuilderProductType<Builder>::ProductT>
        asset(TaskExecutor &executor, Args &&...args)
    {
        const auto it = load_asset<Builder>(
            executor, std::forward<Args>(args)...);
        return make_asset_access_proxy<
            typename AssetBuilderProductType<Builder>::ProductT>(it);
    }

    /*
     * After having the asset id, it can be used to access the asset with
     * better performance (without having to hash the parameters). However,
     * if the asset couldn't be found, the previous function have to be called
     * again.
     */
    template <AssetBuilder Builder>
    AssetAccessProxy<typename AssetBuilderProductType<Builder>::ProductT>
        asset(AssetHashId id)
    {
        const auto it = find_asset(id);
        if(it == mAssetRecords.end())
            return { id, nullptr };
        validate_builder_type<Builder>(it);
        return make_asset_access_proxy<
            typename AssetBuilderProductType<Builder>::ProductT>(it);
    }
};
}
