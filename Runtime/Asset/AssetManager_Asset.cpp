#include "AssetManager2.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/TaskExecutor.hpp>

namespace usagi
{
std::pair<AssetManager2::AssetRecordRef, bool>
AssetManager2::try_emplace(const AssetHashId id)
{
    /*
     * Protects that no entities shall be inserted twice. This operation
     * checks the existence of the asset and only inserts new record when
     * it doesn't exist. Therefore, only one request will successfully insert
     * the asset record, other requests will get the ref to it.
     */
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
    // Atomic variable, no locking needed.
    it->second.status.store(AssetStatus::QUEUED, std::memory_order::release);
}

void AssetManager2::unload_single_asset_nolock(const AssetHashId asset)
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

    const auto it = find_asset_nolock(asset);

    // Ensure that the asset actually exists.
    USAGI_ASSERT_THROW(
        it != mAssetRecords.end(),
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded does not exist.", asset
        ))
    );

    const auto &record = it->second;

    LOG(info, "[Asset] Unloading asset {:#0x}, current status: {}",
        asset, to_string(record.status));

    // The asset can only be unloaded when it is not in loading state.
    USAGI_ASSERT_THROW(
        record.status != AssetStatus::QUEUED &&
        record.status != AssetStatus::LOADING,
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded shall not be in loading state.",
            asset
        ))
    );

    // Ensure that the asset is not in use.
    USAGI_ASSERT_THROW(
        record.ref_tracker.use_count() == 1,
        std::logic_error(std::format(
            "The asset ({:#0x}) to be unloaded is still being used: {}", asset
        ))
    );

    mAssetRecords.erase(it);
}
}
