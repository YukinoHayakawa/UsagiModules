#include "AssetManager2.hpp"

#include <Usagi/Runtime/TaskExecutor.hpp>

#include "details/AssetBuildTask.hpp"

namespace usagi
{
std::pair<AssetManager2::AssetRecordRef, bool>
AssetManager2::try_emplace(const AssetHashId id)
{
    std::unique_lock lk(mAssetTableMutex);
    const auto [it, inserted] = mAssetRecords.try_emplace(id);
    lk.unlock();
    return { it, inserted };
}

void AssetManager2::make_submit_build_task(
    TaskExecutor &executor,
    std::unique_ptr<AssetBuilder> builder,
    const AssetRecordRef it)
{
    auto task = std::make_unique<AssetBuildTask>(
        *this,
        executor,
        it->second.asset,
        it->second.status,
        std::move(builder)
    );
    it->second.future = task->future();
    executor.submit(std::move(task));
    it->second.status.store(AssetStatus::QUEUED, std::memory_order::release);
}

AssetAccessProxy AssetManager2::result_from(AssetRecordRef it)
{
    return { it->first, &it->second };
}

void AssetManager2::add_package(std::unique_ptr<AssetPackage> package)
{
    std::unique_lock lk(mPackageMutex);
    mPackageManager.add_package(std::move(package), 0);
}

void AssetManager2::remove_package(AssetPackage *package)
{
    std::unique_lock lk(mPackageMutex);
    mPackageManager.remove_package(package);
}

AssetQuery * AssetManager2::create_asset_query(
    AssetPath path,
    MemoryArena &arena)
{
    // Note: although iterating through the packages is a read operation,
    // the query can cause modifications to certain package and that
    // is synchronized internally within the package impl.
    std::shared_lock lk(mPackageMutex);
    return mPackageManager.create_query(path, arena);
}

AssetAccessProxy AssetManager2::asset(AssetHashId id)
{
    std::shared_lock lk(mAssetTableMutex);
    const auto it = mAssetRecords.find(id);
    if(it == mAssetRecords.end())
        return { id, nullptr };
    return result_from(it);
}
}
