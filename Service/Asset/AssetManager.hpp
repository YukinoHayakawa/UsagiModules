#pragma once

#include <memory>
#include <set>
#include <mutex>
#include <vector>

#include <Usagi/Library/Memory/LockGuard.hpp>

#include "Asset.hpp"
#include "AssetSource.hpp"
#include "TypeId.hpp"
#include "AssetCacheEntry.hpp"
#include "Crc32.hpp"
#include "Builder/AssetBuilderRawBuffer.hpp"

namespace usagi
{
class AssetManager
{
    struct AssetCacheEntryComparator
    {
        bool operator()(
            const std::shared_ptr<AssetCacheEntry> &lhs,
            const std::shared_ptr<AssetCacheEntry> &rhs) const
        {
            return *lhs < *rhs;
        }

        bool operator()(
            const std::shared_ptr<AssetCacheEntry> &lhs,
            const AssetHandle rhs) const
        {
            return lhs->handle() < rhs;
        }

        bool operator()(
            const AssetHandle lhs,
            const std::shared_ptr<AssetCacheEntry> &rhs) const
        {
            return lhs < rhs->handle();
        }

        using is_transparent = void;
    };

    // This mutex only locks the access to the cache map. Once an entry
    // is accessed, its reference counter is immediately incremented to
    // prevent it from being released.
    std::recursive_mutex mCacheMutex;
    std::set<
        std::shared_ptr<AssetCacheEntry>,
        AssetCacheEntryComparator
    > mAssetCache;

    std::mutex mSourceMutex;
    std::vector<std::unique_ptr<AssetSource>> mSources;

    std::uint64_t mFrameCounter = 0;

    using RefCountedCacheEntry = RefCounted<
        AssetCacheEntry,
        AssetCacheEntry::RefCountTraits
    >;

    RefCountedCacheEntry try_find_entry(const AssetHandle handle)
    {
        std::lock_guard lock(mCacheMutex);

        const auto iter = mAssetCache.find(handle);
        if(iter == mAssetCache.end())
            return { };

        // Wrap the cache entry in a reference counter so it cannot be freed
        // by other threads
        return { iter->get() };
    }

public:
    void add_source(std::unique_ptr<AssetSource> source)
    {
        std::lock_guard lock(mSourceMutex);

        mSources.emplace_back(std::move(source));
    }

    AssetSource * try_find_source(const std::u8string_view locator)
    {
        std::lock_guard lock(mSourceMutex);

        for(std::size_t i = 0; i < mSources.size(); ++i)
        {
            if(mSources[i]->has_asset(locator))
                return mSources[i].get();
        }
        return nullptr;
    }

    // The request to asset returns a reference-counted promise-like object
    // referring to the cache entry. This object internally refers to the
    // cache entry inside the cache map. When the status of the asset is loaded,
    // the object can be used to cast the raw pointer to the actual object
    // stored in the cache. If the asset is loading, the object could be used to
    // wait on the finish of the loading. If the asset is unloaded and no
    // loading action is demanded, an empty handle will be returned.

    RefCountedCacheEntry request_cached_asset(const AssetHandle handle)
    {
        if(auto entry = try_find_entry(handle))
        {
            if(entry->mStatus != AssetStatus::UNINITIALIZED)
                return entry;
        }

        // If there is no cache entry or the asset has been unloaded,
        // a builder must be provided via request_asset() in order to rebuild
        // the asset
        return { };
    }

    template <
        typename Builder = AssetBuilderRawBuffer,
        typename... BuilderArgs
    >
    RefCountedCacheEntry request_asset(
        const AssetPriority priority,
        const bool load_if_unloaded,
        BuilderArgs &&... args) requires(AssetBuilder<Builder>)
    {
        using AssetType = typename Builder::OutputT;

        Builder builder { this, std::forward<BuilderArgs>(args)... };

        // Hash the asset info to create the handle
        std::uint32_t handle = builder.hash();
        handle = crc32c(handle, TYPE_ID<Builder>);

        // ================== CRITICAL SECTION BEGIN ==================

        LockGuard lock(mCacheMutex);

        // Increment the reference
        auto entry = try_find_entry(handle);

        // The entry is in the cache but may be unloaded
        if(entry && entry->mStatus != AssetStatus::UNINITIALIZED)
        {
            return entry;
        }

        if(!load_if_unloaded) return { };

        // If the cache entry is not present and loading the asset is demanded,
        // proceed to loading the asset

        // If this is the first request to the asset, create an entry
        if(!entry)
        {
            auto new_entry = std::make_unique<AssetCacheEntry>();
            new_entry->mTypeId = TYPE_ID<AssetType>;
            // todo what if subsequent requests change this priority?
            new_entry->mPriority = priority;
            new_entry->mHandle = handle;
            entry = new_entry.get();
            const auto i = mAssetCache.emplace(std::move(new_entry));
            assert(i.second);
        }

        assert(entry->mStatus == AssetStatus::UNINITIALIZED);

        // Prevent creation of new loading jobs
        entry->mStatus = AssetStatus::LOADING;

        lock.unlock();

        // ================== CRITICAL SECTION END ==================

        entry->mLoadFuture = std::async(std::launch::async, [
            this, entry, builder = std::move(builder)
        ]() mutable {
            entry->mBlob = builder.build();
            entry->mStatus = AssetStatus::LOADED;
        });

        return entry;
    }

    // Increment the counter used to record last access time by one.
    void tick_frame()
    {
        ++mFrameCounter;
    }
};
}
