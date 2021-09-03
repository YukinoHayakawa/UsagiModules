#include "AssetSourceFilesystem.hpp"

#include <Usagi/Library/Memory/LockGuard.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>

namespace usagi
{
AssetSourceFilesystem::AssetSourceFilesystem(
    const std::filesystem::path &base_path)
    : mBasePath(canonical(base_path))
{
}

bool AssetSourceFilesystem::has_asset(
    const std::u8string_view name) const
{
    const auto file_path = mBasePath / name;

    return exists(file_path);
}

MemoryRegion AssetSourceFilesystem::load(const std::u8string_view name)
{
    std::filesystem::path relative_path = mBasePath / name;
    relative_path = relative_path.lexically_relative(mBasePath);

    // trying to escape the folder
    if(*relative_path.begin() == "..")
        USAGI_THROW(std::runtime_error("File not in specified folder."));

    LockGuard lock(mLock);

    auto iter = mLoadedFiles.find(relative_path);

    if(iter == mLoadedFiles.end())
    {
        iter = mLoadedFiles.try_emplace(
            std::move(relative_path),
            // todo map a read-only view
            RegularFile(mBasePath / relative_path).create_view()
        ).first;
    }

    auto &mapping = iter->second;

    lock.unlock();

    mapping.prefetch();

    return {
        .base_address = mapping.base_view(),
        .length = mapping.max_size()
    };
}

void AssetSourceFilesystem::unload(const std::u8string_view name)
{
    std::filesystem::path relative_path = mBasePath / name;
    relative_path = relative_path.lexically_relative(mBasePath);

    mLoadedFiles.erase(relative_path);
}
}
