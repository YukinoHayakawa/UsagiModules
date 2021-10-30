#pragma once

#include <map>
#include <mutex>

#include <Usagi/Runtime/File/MappedFileView.hpp>

#include "AssetPackage.hpp"

namespace usagi
{
class AssetPackageFilesystem final : public AssetPackage
{
    std::filesystem::path mBasePath;

    // <relative path, memory mapping>
    std::map<std::filesystem::path, MappedFileView> mLoadedFiles;
    using FileEntry = decltype(mLoadedFiles)::iterator;
    std::mutex mLock;

    class Query final : public AssetQuery
    {
        AssetPackageFilesystem *mPackage;
        MappedFileView &mMapping;
        // todo: fix
        bool mFetched = false;

        [[nodiscard]]
        AssetFingerprint fingerprint_impl() override;

    public:
        Query(AssetPackageFilesystem *package, MappedFileView &mapping)
            : mPackage(package)
            , mMapping(mapping)
        {
        }

        [[nodiscard]]
        AssetPackage * package() const override { return mPackage; }

        [[nodiscard]]
        bool prefetched() const override;

        void fetch() override;

        ReadonlyMemoryRegion memory_region() override;
        // void evict() override;
    };

    friend class Query;

public:
    explicit AssetPackageFilesystem(
        const std::filesystem::path &base_path);

    bool create_query(
        std::string_view path,
        StackPolymorphicObject<AssetQuery> &query) override;
};
}
