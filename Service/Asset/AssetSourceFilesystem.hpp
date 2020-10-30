#pragma once

#include <map>
#include <mutex>
#include <string_view>

#include <Usagi/Runtime/File/MappedFileView.hpp>

#include "AssetSource.hpp"

namespace usagi
{
class AssetSourceFilesystem final : public AssetSource
{
    std::filesystem::path mBasePath;

    // <relative path, memory mapping>
    std::map<std::filesystem::path, MappedFileView> mLoadedFiles;
    std::mutex mLock;

public:
    explicit AssetSourceFilesystem(
        const std::filesystem::path &base_path);

    bool has_asset(std::u8string_view name) const override;
    MemoryRegion load(std::u8string_view name) override;
    void unload(std::u8string_view name) override;
};
}
