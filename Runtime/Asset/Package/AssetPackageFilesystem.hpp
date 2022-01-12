#pragma once

#include <map>
#include <mutex>
#include <set>

#include <Usagi/Runtime/File/MappedFileView.hpp>

#include "../AssetPackage.hpp"
#include "../details/AssetDefs.hpp"

namespace usagi
{
class FilesystemWatcher;

class AssetPackageFilesystem final : public AssetPackage
{
    std::unique_ptr<FilesystemWatcher> mWatcher;
    std::string mBasePathStr;
    std::set<std::filesystem::path> mPendingFiles;

    const std::filesystem::path & base_path() const;

    struct FileEntry
    {
        std::uint64_t file_id;
        std::uint64_t file_mod_time;
        AssetHashId asset_id;
        MappedFileView view;
    };

    // <relative path, file meta>
    std::map<std::filesystem::path, FileEntry> mLoadedFiles;
    // using FileEntry = decltype(mLoadedFiles)::iterator;
    std::recursive_mutex mFileTableMutex;

    friend class AssetQueryFilesystem;
    friend class AssetPackageFilesystemFsEventHandler;

    std::filesystem::path normalized_path(AssetPath path) const;

public:
    explicit AssetPackageFilesystem(const std::filesystem::path &base_path);
    ~AssetPackageFilesystem() override;

    ReturnValue<AssetStatus, AssetQuery *> create_query(
        AssetPath path,
        MemoryArena &arena) override;
    bool evict(AssetPath path) override;

    void poll_asset_changes(AssetChangeCallbackProxy &callback) override;

    std::string_view type() const override;
    std::string_view root() const override;
};
}
