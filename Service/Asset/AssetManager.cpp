#include "AssetManager.hpp"

#include <cassert>

#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Library/Memory/LockGuard.hpp>

namespace usagi
{
// todo: LRU memory mgmt

std::optional<std::shared_ptr<MemoryRegion>> AssetManager::request_raw_asset(
    const std::u8string_view locator,
    const AssetPriority priority,
    const AssetRequestOption options)
{
    // Decide which file to load from
    const auto source = find_source(locator);

    if(source == nullptr)
        USAGI_THROW(AssetNotFound(std::u8string(locator)));

    // Construct asset key
    AssetSearchKey key;
    key.source = source;
    key.name = locator;

    LockGuard lock(mCacheMutex);

    // Find asset in cache
    auto cache_iter = mAssetCache.find(key);
    const bool no_entry = cache_iter == mAssetCache.end();

    // If found in cache, return the cached memory.
    if(!no_entry && cache_iter->second->status == AssetStatus::PRESENT)
    {
        return cache_iter->second->blob;
    }

    // Otherwise optionally create a loading job or return a fallback

    if(options & ASSET_LOAD_IF_MISSING)
    {
        // Load the asset if cache entry is not created or unloaded
        if(no_entry || cache_iter->second->status == AssetStatus::UNLOADED)
        {
            std::unique_ptr<AssetCacheEntry> entry;

            if(no_entry)
                entry = std::make_unique<AssetCacheEntry>();
            else // unloaded
                entry = std::move(cache_iter->second);

            // source and entry shall not be removed before the load job
            // is done
            entry->status = AssetStatus::LOADING;
            entry->load_future = std::async(std::launch::async, [
                source, entry = entry.get(), name = std::u8string(key.name)
            ]() {
                // source->load() must be thread-safe
                *entry->blob = source->load(name);
                // after the loading job is done, mark it as present
                entry->status = AssetStatus::PRESENT;
            });

            if(no_entry)
            {
                // Insert cache entry
                const auto insert = mAssetCache.try_emplace(
                    AssetKey(key), std::move(entry)
                );
                cache_iter = insert.first;
                assert(insert.second);
            }
            else // unloaded
                cache_iter->second = std::move(entry);
        }
        // post-cond: cache_iter is guaranteed to point to some entry now
        // and the asset must be in LOADING status.

        // Wait for the job to complete if it is a blocking read.
        if(options & ASSET_BLOCKING_LOAD)
        {
            auto *entry_ptr = cache_iter->second.get();

            // Increment reference
            auto blob = entry_ptr->blob;

            // Don't block on blocking load
            lock.unlock();

            // It doesn't make sense to request a blocking load
            // while using fallback asset.
            assert((options & ASSET_FALLBACK_IF_MISSING) == 0);

            entry_ptr->load_future.wait();
            assert(entry_ptr->status == AssetStatus::PRESENT);

            return blob;
        }

        // Otherwise the asset is unavailable during loading
        return { };
    }

    if(options & ASSET_FALLBACK_IF_MISSING)
    {
        // todo impl
        return { };
    }

    // Do not load or use fallback if the asset is not loaded. The request
    // is essentially ignored. This is usually the case for post-processing
    // systems that depend on previous systems to load the assets.
    return { };
}
}
