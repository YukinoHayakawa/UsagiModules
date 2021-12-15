#pragma once

#include <map>
#include <mutex>

#include <Usagi/Runtime/File/MappedFileView.hpp>

#include "../AssetPackage.hpp"

namespace usagi
{
class FilesystemWatcher;

class AssetPackageFilesystem final : public AssetPackage
{
    std::unique_ptr<FilesystemWatcher> mWatcher;
    std::string mBasePathStr;

    const std::filesystem::path & base_path() const;

    // <relative path, memory mapping>
    std::map<std::filesystem::path, MappedFileView> mLoadedFiles;
    // using FileEntry = decltype(mLoadedFiles)::iterator;
    std::mutex mFileTableMutex;

    friend class AssetQueryFilesystem;
    friend class AssetPackageFilesystemFsEventHandler;

public:
    explicit AssetPackageFilesystem(const std::filesystem::path &base_path);

    AssetQuery * create_query(AssetPath path, MemoryArena &arena) override;

    void report_asset_changes(class AssetManager2 &manager) override
    {
        /* todo */
    }

    std::string_view type() const override;
    std::string_view root() const override;
};
}
