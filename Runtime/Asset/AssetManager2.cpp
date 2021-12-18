#include "AssetManager2.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/TaskExecutor.hpp>

#include "details/AssetQuery.hpp"

namespace usagi
{
std::pair<AssetManager2::AssetRecordRef, bool>
AssetManager2::try_emplace(const AssetHashId id)
{
    std::unique_lock lk(mAssetTableMutex);
    const auto [it, inserted] = mAssetRecords.try_emplace(id);
    lk.unlock();
    LOG(info, "[Asset] Asset entry added: {:#0x}", id);
    return { it, inserted };
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AssetManager2::submit_build_task(
    TaskExecutor &executor,
    std::unique_ptr<Task> build_task,
    const AssetRecordRef it)
{
    executor.submit(std::move(build_task));
    it->second.status.store(AssetStatus::QUEUED, std::memory_order::release);
}

void AssetManager2::add_package(std::unique_ptr<AssetPackage> package)
{
    std::scoped_lock lk(mPackageMutex, mAssetTableMutex, mDependencyGraphMutex);
    // todo unload_overriden_assets(package);
    mPackageManager.add_package(std::move(package));
}

void AssetManager2::remove_package(AssetPackage *package)
{
    std::scoped_lock lk(mPackageMutex, mAssetTableMutex, mDependencyGraphMutex);
    // todo: it's generally impossible to make sure all affected assets are not in use
    // todo unload_derivative_assets(package);
    mPackageManager.remove_package(package);
}

AssetQuery * AssetManager2::create_asset_query(
    AssetHashId requester,
    AssetPath path,
    MemoryArena &arena)
{
    // Note: although iterating through the packages is a read operation,
    // the query can cause modifications to certain package and that
    // is synchronized internally within the package impl.
    std::shared_lock lk(mPackageMutex);
    const auto query = mPackageManager.create_query(path, arena);
    if(query && requester) register_dependency(requester, query->package());
    return query;
}

void AssetManager2::unload_asset_nolock(const AssetHashId id)
{
    /*
     * todo: what if the asset is in use?
     * Possible solutions:
     * 1. Allow users of the asset hold references to the asset. The asset is
     *    destructed/released after they released the references.
     *    Cons: The time when the asset will be released will be unpredictable
     *    thus proper memory management is impractical.
     * 2. Tag the asset as invalidated and find proper opportunity to remove
     *    it (when no reference is held). Cons are similar to 1.
     * 3. Require that no asset shall be referenced when performing invalidating
     *    operations.
     *    Cons: Will stall the pipeline too much.
     * 4. Require that no users (Systems) shall hold references to assets
     *    for extended frame of time.
     *    Cons: How to enforce? Force asset reference construction on the
     *    stack?
     * Discussion:
     * The following solutions all rely on the assumption that no asset shall
     * hold references to other asset except during construction. In this way,
     * each asset can theoretically be destructed without having to unload its
     * derivative assets first. A dependency edge only describes the
     * relationship that if an asset is updated, the asset being pointed to
     * shall be updated, too. It does not imply memory ownership.
     */

    const AssetRecordRef it = find_asset_nolock(id);

    // Ensure that the asset actually exists.
    USAGI_ASSERT_THROW(
        it != mAssetRecords.end(),
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded does not exist.", id
        ))
    );

    const auto &record = it->second;

    LOG(info, "[Asset] Unloading asset {:#0x}, current status: {}",
        id, to_string(record.status));

    // The asset can only be unloaded when it is not in loading state.
    USAGI_ASSERT_THROW(
        record.status != AssetStatus::QUEUED &&
        record.status != AssetStatus::LOADING,
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded shall not be in loading state.", id
        ))
    );

    // Ensure that the asset is not in use.
    USAGI_ASSERT_THROW(
        record.ref_tracker.use_count() == 1,
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded is still being used: {}", id
        ))
    );

    mAssetRecords.erase(it);
}

void AssetManager2::register_dependency(
    AssetHashId requester,
    AssetHashId target)
{
    LOG(trace, "[Asset] Adding edge Asset {:#0x} -> Asset {:#0x}", target, requester);
    std::unique_lock lk(mDependencyGraphMutex);
    mDependencies[target].emplace(requester);
    mDependencyReversed[requester].emplace(target);
}

void AssetManager2::register_dependency(
    AssetHashId requester,
    AssetPackage *target)
{
    LOG(trace, "[Asset] Adding edge Package {:#0x} -> Asset {:#0x}",
        (std::uint64_t)target, requester);
    std::unique_lock lk(mDependencyGraphMutex);
    mDependencies[target].emplace(requester);
    mDependencyReversed[requester].emplace(target);
}

bool AssetManager2::has_dependency_edge(
    const Vertex from,
    const AssetHashId to)
{
    std::shared_lock lk(mDependencyGraphMutex);
    const auto it = mDependencies.find(from);
    if(it == mDependencies.end()) return false;
    return it->second.contains(to);
}

void AssetManager2::erase_dependency_edge(Vertex from, AssetHashId to)
{
    const auto it = mDependencies.find(from);
    assert(it != mDependencies.end());
    // LOG(debug, "[Asset] Removing edge: {:#0x} -> {:#0x}", from, to);

    if(std::holds_alternative<AssetHashId>(from))
        LOG(debug, "[Asset] Removing dependency: {:#0x} -> {:#0x}",
            std::get<AssetHashId>(from), to);
    else
        LOG(debug, "[Asset] Removing dependency: {:#0x} -> {:#0x}",
            (std::uint64_t)std::get<AssetPackage *>(from), to);

    it->second.erase(to);
    // if(it->second.empty()) mDependencies.erase(it);
}

void AssetManager2::unload_derivative_assets(AssetHashId asset)
{
    std::scoped_lock lk(mDependencyGraphMutex, mAssetTableMutex);

    LOG(debug, "[Asset] Unloading asset and derivatives: {:#0x}", asset);

    USAGI_ASSERT_THROW(
        mAssetRecords.contains(asset),
        std::runtime_error(std::format(
            "[Asset] Asset to be reloaded doesn't exist: {:#0x}", asset
        ))
    );
    // Unload assets depending on the current one.
    unload_derivative_assets_nolock(asset);
}

void AssetManager2::unload_derivative_assets_nolock(AssetHashId asset)
{
    // Recursively unload all derivative assets.

    // Get all outgoing edges.
    const auto out_edges = mDependencies.find(asset);

    // Leaf assets do not have edges to other assets.
    if(out_edges != mDependencies.end())
    {
        // If the asset points to other assets, the assets being pointed to are
        // depending on the current asset, unload them first. (Note that they
        // should not hold any reference to the current asset.)
        while(!out_edges->second.empty())
        {
            const auto &to = *out_edges->second.begin();
            assert(std::holds_alternative<AssetHashId>(to));
            const auto to_id = std::get<AssetHashId>(to);
            LOG(debug, "[Asset] Unloading derivative: {:#0x} -> {:#0x}", asset, to_id);
            unload_derivative_assets_nolock(to_id);
        }
        mDependencies.erase(out_edges);
    }

    // Erase incoming edges.

    // First, find out which vertices point to the current asset.
    const auto it_rev = mDependencyReversed.find(asset);
    assert(it_rev != mDependencyReversed.end());

    // Remove their edges to here.
    for(auto &&in : it_rev->second)
    {
        erase_dependency_edge(in, asset);
    }

    // Delete the reverse edges.
    mDependencyReversed.erase(it_rev);

    unload_asset_nolock(asset);
}

AssetManager2::AssetRecordRef AssetManager2::find_asset_nolock(AssetHashId id)
{
    const auto it = mAssetRecords.find(id);
    return it;
}

AssetManager2::AssetRecordRef AssetManager2::find_asset(AssetHashId id)
{
    std::shared_lock lk(mAssetTableMutex);
    return find_asset_nolock(id);
}
}
