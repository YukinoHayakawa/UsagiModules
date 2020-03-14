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
    struct CacheHandleComparator
    {
        bool operator()(
            const AssetCacheEntry &lhs,
            const AssetCacheEntry &rhs) const
        {
            return lhs.mHandle < rhs.mHandle;
        }

        bool operator()(
            const AssetHandle &lhs,
            const AssetCacheEntry &rhs) const
        {
            return lhs < rhs.mHandle;
        }

        bool operator()(
            const AssetCacheEntry &lhs,
            const AssetHandle &rhs) const
        {
            return lhs.mHandle < rhs;
        }

        using is_transparent = void;
    };

    // This mutex only locks the access to the cache map. Once an entry
    // is accessed, its reference counter is immediately incremented to
    // prevent it from being released.
    std::recursive_mutex mCacheMutex;
    std::set<AssetCacheEntry, CacheHandleComparator> mAssetCache;

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
        // by other threads.
        // We are sure that the asset handle won't be modified through the ref
        // counter wrapper so the invariant of the search tree is safe.
        return { &const_cast<AssetCacheEntry&>(*iter) };
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
            // todo what if subsequent requests change this priority?
            const auto i = mAssetCache.emplace(
                TYPE_ID<AssetType>,
                handle,
                priority
            );
            entry = &const_cast<AssetCacheEntry&>(*i.first);
            assert(i.second);
        }

        assert(entry->mStatus == AssetStatus::UNINITIALIZED);

        // Prevent creation of new loading jobs
        entry->mStatus = AssetStatus::LOADING;

        lock.unlock();

        // ================== CRITICAL SECTION END ==================

        // The lifetime of the lambda passed to async() might be the same
        // as the returned future object. The loader job should hold a
        // reference to the asset but should release it after finishing the
        // task. Therefore the ref count handle is moved into the scope of
        // the function so it get released when the function finished.
        // https://stackoverflow.com/questions/49505280/what-is-the-lifetime-of-the-arguments-of-stdasync/51358564
        entry->mLoadFuture = std::async(std::launch::async, [
            this, entry, builder = std::move(builder)
        ]() mutable {
            auto ref = std::move(entry);
            ref->mBlob = builder.build();
            ref->mStatus = AssetStatus::LOADED;
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
