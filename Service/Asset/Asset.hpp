#pragma once

#include <string>
#include <stdexcept>

#include <Usagi/Runtime/Memory/Region.hpp>

namespace usagi
{
enum class AssetPriority : std::uint8_t
{
    CRITICAL_GAMEPLAY   = 3,
    NORMAL              = 2,
    LOW_DECORATIVE      = 1,
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

// CRC32C value of the asset info
using AssetHandle = std::uint32_t;

template <typename T>
concept AssetBuilder = requires (T t) {
    { T::OutputT };
    // Cache identifier is a number used together with the processor type
    // to index the processed asset caches. The identifier should reflect the
    // parameters of the processor. For example, for an image decoder, the
    // identifier should uniquely reflect the pixel formal, number of channels,
    // etc. For video decoder, it should be able to index the caches by video
    // chunks so the video could be partially loaded.
    { t.hash() } -> AssetHandle;
    { t.build() } -> MemoryRegion;
    { T::free(std::declval<const MemoryRegion &>()) };
};

enum class AssetStatus : std::uint8_t
{
    // This status is present during the creation of the cache entry or it
    // can mean that the cache was removed due to memory management.
    // The reason that the entry is not purged is to audit the memory
    // usage and detect potential cache thrashing by examining the
    // last access time, etc.
    UNINITIALIZED,

    // This status indicates there is an active job for loading the
    // content of the asset thus no second job shall be created for it.
    LOADING,

    // This status indicates that there is an active copy of the asset
    // in the memory.
    LOADED,
};
}
