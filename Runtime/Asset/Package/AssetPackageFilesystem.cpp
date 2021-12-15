#include "AssetPackageFilesystem.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>
#include <Usagi/Runtime/File/FilesystemWatcher.hpp>

#include "../details/AssetQuery.hpp"

namespace usagi
{
class AssetQueryFilesystem final : public AssetQuery
{
    AssetPackageFilesystem *mPackage;
    MappedFileView &mMapping;
    bool mFetched = false;

public:
    AssetQueryFilesystem(
        AssetPackageFilesystem *package,
        MappedFileView &mapping)
        : mPackage(package)
        , mMapping(mapping)
    {
    }

    AssetPackage * package() const override { return mPackage; }

    void fetch() override
    {
        mMapping.prefetch();
        mFetched = true;
    }

    bool ready() const override { return mFetched; }
    ReadonlyMemoryView data() override { return mMapping.memory_region(); }
    // void evict() override;
};

// void AssetQueryFilesystem::evict()
// {
//
// }

/*
class AssetPackageFilesystemFsEventHandler : public FilesystemEventHandler
{
    AssetManager2 *mManager;
    AssetPackageFilesystem *mPackage;

    void unload_asset(const std::filesystem::path &asset_path);
public:
    void file_added(
        std::uint64_t file_id,
        const std::filesystem::path &path) override;
    void file_removed(
        std::uint64_t file_id,
        const std::filesystem::path &path) override;
    void file_changed(
        std::uint64_t file_id,
        const std::filesystem::path &path) override;
    void file_renamed(
        std::uint64_t file_id,
        const std::filesystem::path &old_path,
        const std::filesystem::path &new_path) override;
    void folder_added(
        std::uint64_t file_id,
        const std::filesystem::path &path) override;
    void folder_removed(
        std::uint64_t file_id,
        const std::filesystem::path &path) override;
    void folder_renamed(
        std::uint64_t file_id,
        const std::filesystem::path &old_path,
        const std::filesystem::path &new_path) override;
    void out_of_sync() override;
};

ReadonlyMemoryView AssetQueryFilesystem::data()
{
    return mMapping.memory_region();
}

void AssetPackageFilesystemFsEventHandler::unload_asset(
    const std::filesystem::path &asset_path)
{
    mManager->unload_asset_from_package(mPackage, asset_path.string());
}

void AssetPackageFilesystemFsEventHandler::file_added(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    auto asset_path = path;

    // Handle signals for updating an existing asset.
    const auto asset_name = asset_path.filename();
    const auto asset_name_str = asset_name.string();
    // A specific sequence that is usually not unintentionally input. Maybe
    // make this configurable.
    constexpr std::string_view feature_seq = "!~!";
    if(asset_name_str.starts_with(feature_seq))
    {
        // Recover the name of the asset to be updated.
        const auto original_name = asset_name_str.substr(feature_seq.size());
        if(original_name.empty())
        {
            LOG(warn, "[AssetPkgFs] A file whose name is only the update "
                "signal was added.");
            return;
        }
        asset_path.replace_filename(original_name);

        // todo handle outstanding references to the asset
        const bool unloaded = mManager->unload_asset_from_package(
            this, asset_path.string());
        if(unloaded)
        {
            // Overwrite the asset file to be updated. The content of the old
            // asset is not preserved here because the semantic of this update
            // operation is to replace the content of the old asset. The
            // replacement of the old asset file is deferred to here only
            // because the old file may be memory-mapped and thus could not be
            // modified by external programs. So the user has to follow a
            // certain protocol to notify us that the file is to be replaced by
            // creating a file with the same name of the old asset but
            // with a special prefixed.
            if(exists(asset_path))
            {
                rename(path, asset_path);
                LOG(info, "[AssetPkgFs] Asset {} was unloaded and overwritten.",
                    asset_path);
            }
        }
    }
    else
    {
        // Figure out if any loaded asset is overriden by this new file.
        // If such a file is found, unload it.
        mManager->purge_overriden_asset(path.string());
    }
}

void AssetPackageFilesystemFsEventHandler::file_removed(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    unload_asset(path);
}

void AssetPackageFilesystemFsEventHandler::file_changed(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    unload_asset(path);
}

void AssetPackageFilesystemFsEventHandler::file_renamed(
    std::uint64_t file_id,
    const std::filesystem::path &old_path,
    const std::filesystem::path &new_path)
{
    unload_asset(old_path);
    file_added(file_id, new_path);
}

void AssetPackageFilesystemFsEventHandler::folder_added(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    mManager->purge_overriden_assets(path.string());
}

void AssetPackageFilesystemFsEventHandler::folder_removed(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    mManager->unload_subtree_from_package(mPackage, path.string());
}

void AssetPackageFilesystemFsEventHandler::folder_renamed(
    std::uint64_t file_id,
    const std::filesystem::path &old_path,
    const std::filesystem::path &new_path)
{
    mManager->unload_subtree_from_package(mPackage, old_path.string());
    mManager->purge_overriden_assets(new_path.string());
}

void AssetPackageFilesystemFsEventHandler::out_of_sync()
{
    mManager->purge_package_assets(mPackage);
}
*/

const std::filesystem::path & AssetPackageFilesystem::base_path() const
{
    return mWatcher->base_folder();
}

AssetPackageFilesystem::AssetPackageFilesystem(
    const std::filesystem::path &base_path)
    : mWatcher(create_filesystem_watcher(canonical(base_path)))
    , mBasePathStr(this->base_path().string())
{
}

AssetPackageFilesystem::~AssetPackageFilesystem()
{
}

AssetQuery * AssetPackageFilesystem::create_query(
    AssetPath path,
    MemoryArena &arena)
{
    std::filesystem::path relative_path = base_path() / path.reconstructed();

    // If base path is empty, the package is allowed to access the full
    // filesystem.
    // todo: test
    if(!base_path().empty())
        relative_path = relative_path.lexically_relative(base_path());
    relative_path = relative_path.lexically_normal();

    // forbid attempts to escape the folder
    if(*relative_path.begin() == "..")
        USAGI_THROW(std::runtime_error("Path points to outside the folder."));

    std::unique_lock lock(mFileTableMutex);

    auto iter = mLoadedFiles.find(relative_path);
    if(iter == mLoadedFiles.end())
    {
        try
        {
            iter = mLoadedFiles.try_emplace(
                std::move(relative_path),
                // todo map a read-only view
                RegularFile(base_path() / relative_path).create_view()
            ).first;
        }
        // File could not be open. todo maybe log the reason?
        catch(const std::runtime_error &)
        {
            return nullptr;
        }
    }

    return arena.create<AssetQueryFilesystem>(this, iter->second);
}

std::string_view AssetPackageFilesystem::type() const
{
    return "Filesystem";
}

std::string_view AssetPackageFilesystem::root() const
{
    return mBasePathStr;
}
}
