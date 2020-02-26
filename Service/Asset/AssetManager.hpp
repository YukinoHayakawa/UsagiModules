#pragma once

#include <memory>
#include <optional>
#include <string>
#include <map>
#include <future>

#include <Usagi/Library/Memory/SpinLock.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>

#include "AssetSource.hpp"

namespace usagi
{
enum class AssetPriority
{
    CRITICAL_GAMEPLAY,
    NORMAL,
    LOW_DECORATIVE,
};

enum AssetRequestOption
{
    ASSET_FALLBACK_IF_MISSING   = 1 << 1,
    ASSET_LOAD_IF_MISSING       = 1 << 2,
    ASSET_BLOCKING_LOAD         = 1 << 3,
};

struct AssetNotFound final : std::runtime_error
{
    const std::u8string locator;

    explicit AssetNotFound(std::u8string locator)
        : runtime_error("Requested asset was not found in any asset source.")
        , locator(std::move(locator))
    {
    }
};

class AssetManager
{
    enum class AssetStatus
    {
        UNLOADED,
        LOADING,
        PRESENT,
    };

    struct AssetSearchKey
    {
        AssetSource *source = nullptr;
        std::u8string_view name;
    };

    struct AssetKey
    {
        AssetSource *source = nullptr;
        std::u8string name;

        explicit AssetKey(const AssetSearchKey search_key)
            : source(search_key.source)
            , name(search_key.name)
        {
        }
    };

    struct AssetKeyComparator
    {
        // allow searching with AssetSearchKey
        // https://www.fluentcpp.com/2017/06/09/search-set-another-type-key/
        using is_transparent = void;

        constexpr bool operator()(
            const AssetKey &lhs,
            const AssetKey &rhs) const
        {
            if(lhs.source < rhs.source)
                return true;
            if(rhs.source < lhs.source)
                return false;
            return lhs.name < rhs.name;
        }

        constexpr bool operator()(
            const AssetKey &lhs,
            const AssetSearchKey &rhs) const
        {
            if(lhs.source < rhs.source)
                return true;
            if(rhs.source < lhs.source)
                return false;
            return lhs.name < rhs.name;
        }

        constexpr bool operator()(
            const AssetSearchKey &lhs,
            const AssetKey &rhs) const
        {
            if(lhs.source < rhs.source)
                return true;
            if(rhs.source < lhs.source)
                return false;
            return lhs.name < rhs.name;
        }
    };

    struct AssetCacheEntry
    {
        // Cache status
        AssetStatus status = AssetStatus::UNLOADED;
        std::future<void> load_future;
        std::shared_ptr<MemoryRegion> blob = std::make_unique<MemoryRegion>();

        // Decide which asset to evict
        // todo: impl LRU eviction policy
        AssetPriority priority = AssetPriority::NORMAL;
        std::size_t last_access = 0;
    };

    std::mutex mSourceMutex;
    std::vector<std::unique_ptr<AssetSource>> mSources;

    std::mutex mCacheMutex;
    // todo manage dynamic memory
    std::map<
        AssetKey,
        std::unique_ptr<AssetCacheEntry>,
        AssetKeyComparator
    > mAssetCache;
    std::uint64_t mFrameCounter = 0;

    AssetSource * find_source(const std::u8string_view locator)
    {
        LockGuard lock(mSourceMutex);

        for(std::size_t i = 0; i < mSources.size(); ++i)
        {
            if(mSources[i]->has_asset(locator))
                return mSources[i].get();
        }
        return nullptr;
    }

public:
    void add_source(std::unique_ptr<AssetSource> source)
    {
        LockGuard lock(mSourceMutex);

        mSources.emplace_back(std::move(source));
    }

    std::optional<std::shared_ptr<MemoryRegion>> request_raw_asset(
        std::u8string_view locator,
        AssetPriority priority,
        AssetRequestOption options);

    // Increment the counter used to record last access time by one.
    void tick_frame()
    {
        ++mFrameCounter;
    }
};
}
