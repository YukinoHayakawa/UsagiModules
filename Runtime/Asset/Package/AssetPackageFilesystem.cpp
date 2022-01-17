#include "AssetPackageFilesystem.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/File/RegularFile.hpp>
#include <Usagi/Runtime/File/FilesystemWatcher.hpp>

#include "../details/AssetQuery.hpp"
#include "../details/AssetChangeCallbackProxy.hpp"

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

class AssetPackageFilesystemFsEventHandler final : public FilesystemEventHandler
{
    AssetChangeCallbackProxy &mCallback;
    AssetPackageFilesystem *mPackage;
    // std::size_t mNumChangedAssets = 0;

    void file_outdated(
        std::uint64_t file_id,
        const std::filesystem::path &asset_path) const;
    void handle_changed_file(
        std::uint64_t file_id,
        const std::filesystem::path &path) const;

public:
    AssetPackageFilesystemFsEventHandler(
        AssetChangeCallbackProxy &callback,
        AssetPackageFilesystem *package)
        : mCallback(callback)
        , mPackage(package)
    {
    }

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
    void folder_renamed(
        std::uint64_t file_id,
        const std::filesystem::path &old_path,
        const std::filesystem::path &new_path) override;
    void out_of_sync() override;
};

void AssetPackageFilesystemFsEventHandler::file_outdated(
    std::uint64_t file_id,
    const std::filesystem::path &asset_path) const
{
    const auto path_str = asset_path.string();
    const auto relative_path = mPackage->normalized_path(path_str);

    // const auto iter = mPackage->mLoadedFiles.find(relative_path);
    // // Ignore the event if we haven't load the file.
    // if(iter == mPackage->mLoadedFiles.end())
    //     return;
    // Maybe the asset manager holds only a stub and has released the file
    // handle but wants to be notified when the asset is changed? Like
    // script codes, which can be released after compilation.
    LOG(info, "[AssetPkgFs] Unloading asset: {}", path_str);

    // if(iter->second.file_id != file_id)
    //     LOG(warn, "[AssetPkgFs] File id doesn't match! "
    //         "Loaded={:#0x}, Event={:#0x}",
    //         iter->second.file_id, file_id);

    mCallback.unload_asset(path_str, mPackage);
}

void AssetPackageFilesystemFsEventHandler::handle_changed_file(
    std::uint64_t file_id,
    const std::filesystem::path &path) const
{
    LOG(info, "[AssetPkgFs] Detected new/updated file: {}", path.string());

    // PATH (Root/AssetName.ext)
    auto asset_path = path;

    // Handle signals for updating an existing asset.

    // Extract the file name.
    const auto asset_name = asset_path.filename();
    const auto asset_name_str = asset_name.string();

    // A specific sequence that is usually not unintentionally input. Maybe
    // make this configurable.
    constexpr std::string_view cmd_seq = "!~!";

    // Process the update command.
    if(asset_name_str.starts_with(cmd_seq))
    {
        // Recover the name of the asset to be updated.
        const auto original_name = asset_name_str.substr(cmd_seq.size());
        if(original_name.empty())
        {
            LOG(warn, "[AssetPkgFs] File updates nothing: {} ", path.string());
            return;
        }
        asset_path.replace_filename(original_name);

        LOG(info, "[AssetPkgFs] The new file directs to update asset: {}",
            asset_path.string());

        // todo handle outstanding references to the asset
        // Unload the old asset first.
        if(mCallback.unload_asset({ asset_path.string() }, mPackage))
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
                const auto mod_time = last_write_time(asset_path);
                // "original_asset_backup_YYYY-MM-DD_HH-MM-SS.png"
                const auto backup_name = fmt::format(
                    // "{}_backup_{:%Y-%m-%d_%H-%M-%S}.{}",
                    // todo format time properly
                    "{}_backup_{}{}",
                    asset_path.stem().string(),
                    std::chrono::system_clock::to_time_t(
                        std::chrono::clock_cast<std::chrono::system_clock>(
                            mod_time)),
                    asset_path.extension().string()
                );
                auto backup_path = asset_path;
                backup_path.replace_filename(backup_name);

                LOG(warn, "[AssetPkgFs] Old file backed up to: {} ",
                    backup_path.string());

                platform::file::replace_file(
                    asset_path,
                    path,
                    true,
                    backup_path
                );

                LOG(info, "[AssetPkgFs] Asset {} was unloaded and overwritten.",
                    asset_path);
            }
        }
    }
    else
    {
        // Figure out if any loaded asset is overriden by this new file.
        // If such a file is found, unload it.
        LOG(info, "[AssetPkgFs] Checking for overridden asset.");
        mCallback.unload_overriden_asset(path.string());
    }
}

void AssetPackageFilesystemFsEventHandler::file_added(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    /*
     * File add event happens in two cases:
     * 1. File moved into the watched folder from outside the folder within
     *    the same fs. No subsequent event will happen.
     * 2. A file is being written. File changed events will follow.
     */

    handle_changed_file(file_id, path);
}

void AssetPackageFilesystemFsEventHandler::file_removed(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{
    file_outdated(file_id, path);
}

void AssetPackageFilesystemFsEventHandler::file_changed(
    std::uint64_t file_id,
    const std::filesystem::path &path)
{

    handle_changed_file(file_id, path);
    // file_outdated(path);
}

void AssetPackageFilesystemFsEventHandler::file_renamed(
    std::uint64_t file_id,
    const std::filesystem::path &old_path,
    const std::filesystem::path &new_path)
{
    file_outdated(file_id, old_path);
    file_added(file_id, new_path);
}

// void AssetPackageFilesystemFsEventHandler::folder_added(
//     std::uint64_t file_id,
//     const std::filesystem::path &path)
// {
//     mCallback.unload_overriden_subtree(path.string());
// }
//
// void AssetPackageFilesystemFsEventHandler::folder_removed(
//     std::uint64_t file_id,
//     const std::filesystem::path &path)
// {
//     mCallback.unload_subtree(path.string(), mPackage);
// }

void AssetPackageFilesystemFsEventHandler::folder_renamed(
    std::uint64_t file_id,
    const std::filesystem::path &old_path,
    const std::filesystem::path &new_path)
{
    mCallback.unload_subtree(old_path.string(), mPackage);
    mCallback.unload_overriden_subtree(new_path.string());
}

void AssetPackageFilesystemFsEventHandler::out_of_sync()
{
    mCallback.unload_package_assets(mPackage);
    // treat as removing & adding the package.
}

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

std::filesystem::path AssetPackageFilesystem::normalized_path(
    AssetPath path) const
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

    return relative_path;
}

ReturnValue<AssetStatus, AssetQuery *> AssetPackageFilesystem::create_query(
    AssetPath path,
    MemoryArena &arena)
{
    auto relative_path = normalized_path(path);

    std::unique_lock lock(mFileTableMutex);

    auto iter = mLoadedFiles.find(relative_path);
    if(iter == mLoadedFiles.end())
    {
        try
        {
            const auto abs_path = base_path() / relative_path;
            RegularFile file(abs_path);
            FileEntry entry {
                .file_id = file.id(),
                .file_mod_time = file.last_modification_time(),
                .asset_id = 0,
                // todo map a read-only view
                .view = file.create_view()
            };
            bool inserted;
            std::tie(iter, inserted) = mLoadedFiles.try_emplace(
                std::move(relative_path),
                std::move(entry)
            );
            assert(inserted);
        }
        catch(const platform::file::ExceptionFileNotFound &)
        {
            return { AssetStatus::MISSING, nullptr };
        }
        catch(const platform::file::ExceptionFileAccessDenied &)
        {
            LOG(warn, "[AssetPkgFs] Could not open file {}: Access denied.",
                path.reconstructed());
            return { AssetStatus::MISSING, nullptr };
        }
        catch(const platform::file::ExceptionFileBusy &)
        {
            return { AssetStatus::EXIST_BUSY, nullptr };
        }
        catch(...)
        {
            throw;
        }
    }

    return {
        AssetStatus::EXIST,
        arena.create<AssetQueryFilesystem>(this, iter->second.view)
    };
}

bool AssetPackageFilesystem::evict(AssetPath path)
{
    const auto relative_path = normalized_path(path);

    std::unique_lock lock(mFileTableMutex);

    const auto iter = mLoadedFiles.find(relative_path);
    if(iter == mLoadedFiles.end())
        return false;
    // todo verify the file is not used? add rc?
    mLoadedFiles.erase(iter);
    return true;
}

void AssetPackageFilesystem::poll_asset_changes(
    AssetChangeCallbackProxy &callback)
{
    std::unique_lock lock(mFileTableMutex);

    AssetPackageFilesystemFsEventHandler handler(callback, this);
    // AssetManager may finally call into evict() so the current operation
    // should be lock protected.
    mWatcher->poll_changes(handler);
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
