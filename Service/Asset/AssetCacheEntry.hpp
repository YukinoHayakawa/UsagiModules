#pragma once

#include <atomic>
#include <future>
#include <cassert>

#include <Usagi/Runtime/Memory/Region.hpp>

#include "Asset.hpp"
#include "TypeId.hpp"
#include "RefCounted.hpp"

namespace usagi
{
class AssetCacheEntry
{
    friend class AssetManager;
    std::future<void> mLoadFuture;

    // // Indicates the type of memory allocator. <--- use custom delete function?
    // enum class HeapType
    // {
    //     // The pointer refers to a memory-mapped region backed by a
    //     // file on the disk.
    //     MEMORY_MAPPED,
    //     // The pointer refers to a heap allocator.
    //     HEAP_ALLOCATED,
    //     UNINITIALIZED,
    // } mHeapType = HeapType::UNINITIALIZED;
    //
    // 16 bytes
    MemoryRegion mBlob;

    // A unique code of the stored asset type generated using
    // TypeId template
    //
    // 8 bytes
    void *mTypeId;

    // CRC32 hash of the asset info. Used as a search key for the cache.
    //
    // 4 bytes
    const std::uint32_t mHandle = 0;

    // Counts the active references to this asset. Meant for preventing
    // the cache from being purged while in use. DOES NOT meant to be a
    // mechanism for sharing the ownership of the asset cache. The clients
    // MUST immediately release the reference after the usage of assets,
    // even it is going to used soon in the next frame. When it is the case,
    // make another request to the asset.
    //
    // 4 bytes
    std::atomic<std::uint32_t> mReferenceCounter = 0;

    // The frame index of last access request. Used together with priority
    // to decide entries to purge when the memory is scarce.
    // todo impl usage
    //
    // 4 bytes
    std::uint32_t mLastAccess = 0;

    // Indicates the availability of the asset.
    //
    // 1 byte
    AssetStatus mStatus = AssetStatus::UNINITIALIZED;

    // Loading and memory priority. Higher priority means shorted waiting
    // time of loading and lower change of being evicted from the cache
    // when the memory is scarce.
    // todo impl relevant logic
    //
    // 1 byte
    AssetPriority mPriority = AssetPriority::NORMAL;

public:
    AssetCacheEntry() = default;

    AssetCacheEntry(
        void *type_id,
        const std::uint32_t handle,
        const AssetPriority priority)
        : mTypeId(type_id)
        , mHandle(handle)
        , mPriority(priority)
    {
    }

    template <typename AssetType>
    AssetType * cast()
    {
        // constexpr auto a = sizeof(AssetCacheEntry);
        // todo this could be an assertion
        if(mTypeId != TYPE_ID<AssetType>)
            throw std::runtime_error("invalid asset type cast");
        assert(mStatus == AssetStatus::LOADED);
        return static_cast<AssetType *>(mBlob.base_address);
    }

    MemoryRegion blob() const
    {
        return mBlob;
    }

    bool ready() const
    {
        // Because we hold one reference to the entry, once its loaded,
        // it remains so.
        return mStatus == AssetStatus::LOADED;
    }

    void wait() const
    {
        mLoadFuture.wait();
    }

    std::uint32_t handle() const
    {
        return mHandle;
    }

    struct RefCountTraits
    {
        static std::uint32_t increment_reference(AssetCacheEntry *entry)
        {
            return ++entry->mReferenceCounter;
        }

        static std::uint32_t decrement_reference(AssetCacheEntry *entry)
        {
            return --entry->mReferenceCounter;
        }

        static void free(AssetCacheEntry *entry)
        {
            printf("asset freed!\n");
        }
    };
};
}
