#include "AssetPackageFilesystem.hpp"

#include <format>

#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>

namespace usagi
{
bool AssetPackageFilesystem::Query::prefetched() const
{
    return mFetched;
}

void AssetPackageFilesystem::Query::fetch()
{
    mMapping.prefetch();
    mFetched = true;
}

AssetFingerprint AssetPackageFilesystem::Query::fingerprint_impl()
{
    return hash_memory_region(memory_region());
}

ReadonlyMemoryRegion AssetPackageFilesystem::Query::memory_region()
{
    return mMapping.memory_region();
}
//
// void AssetPackageFilesystem::Query::evict()
// {
//
// }

AssetPackageFilesystem::AssetPackageFilesystem(
    const std::filesystem::path &base_path)
    : mBasePath(canonical(base_path))
{
}

bool AssetPackageFilesystem::create_query(
    std::string_view path,
    StackPolymorphicObject<AssetQuery> &query)
{
    std::filesystem::path relative_path = mBasePath / path;

    // If base path is empty, the package is allowed to access the full
    // filesystem.
    // todo: test
    if(!mBasePath.empty())
        relative_path = relative_path.lexically_relative(mBasePath);
    relative_path = relative_path.lexically_normal();

    // trying to escape the folder
    if(*relative_path.begin() == "..")
        USAGI_THROW(std::runtime_error("Path points to outside the folder."));

    LockGuard lock(mLock);

    auto iter = mLoadedFiles.find(relative_path);
    if(iter == mLoadedFiles.end())
    {
        try
        {
            iter = mLoadedFiles.try_emplace(
                std::move(relative_path),
                // todo map a read-only view
                RegularFile(mBasePath / relative_path).create_view()
            ).first;
        }
        catch(const std::runtime_error &)
        {
            return false;
        }
    }

    lock.unlock();

    // bug: can't ensure that the mapped file view still exists after the query has been created (eg it may be removed from another thread)
    query.construct<Query>(this, iter->second);

    return true;
}

std::string AssetPackageFilesystem::name() const
{
    return std::format("FilesystemFolder: {}", mBasePath.string());
}

//
// bool AssetPackageFilesystem::has_asset(
//     const std::u8string_view name) const
// {
//     const auto file_path = mBasePath / name;
//
//     return exists(file_path);
// }
//
// MemoryRegion AssetPackageFilesystem::load(const std::u8string_view name)
// {
//
// }
//
// void AssetPackageFilesystem::unload(const std::u8string_view name)
// {
//     std::filesystem::path relative_path = mBasePath / name;
//     relative_path = relative_path.lexically_relative(mBasePath);
//
//     mLoadedFiles.erase(relative_path);
// }
}
