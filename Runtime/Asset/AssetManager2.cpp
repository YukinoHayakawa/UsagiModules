#include "AssetManager2.hpp"

#include <Usagi/Runtime/TaskExecutor.hpp>

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

AssetManager2::AssetRecordRef AssetManager2::find_asset(AssetHashId id)
{
    std::shared_lock lk(mAssetTableMutex);
    const auto it = mAssetRecords.find(id);
    return it;
}
}
