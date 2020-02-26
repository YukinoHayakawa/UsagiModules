#include "AssetSourceFilesystemDirectory.hpp"

#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>

namespace usagi
{
AssetSourceFilesystemDirectory::AssetSourceFilesystemDirectory(
    const std::filesystem::path &base_path)
    : mBasePath(canonical(base_path))
{
}

bool AssetSourceFilesystemDirectory::has_asset(
    const std::u8string_view name) const
{
    const auto file_path = mBasePath / name;

    return exists(file_path);
}

MemoryRegion AssetSourceFilesystemDirectory::load(const std::u8string_view name)
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
        RegularFile file {
            (mBasePath / relative_path).u8string(),
            platform::file::OPEN_READ,
            { }
        };

        iter = mLoadedFiles.try_emplace(
            std::move(relative_path),
            std::make_unique<MemoryMappedFile>(
                std::move(file),
                platform::file::MAPPING_READ,
                0 // use file size
            )
        ).first;
    }

    auto mapping = iter->second.get();

    lock.unlock();

    mapping->prefetch(0, mapping->file().size());

    return {
        .base_address = mapping->base_view(),
        .length = mapping->file().size()
    };
}

void AssetSourceFilesystemDirectory::unload(const std::u8string_view name)
{
    std::filesystem::path relative_path = mBasePath / name;
    relative_path = relative_path.lexically_relative(mBasePath);

    mLoadedFiles.erase(relative_path);
}
}
