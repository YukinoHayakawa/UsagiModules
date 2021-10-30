#pragma once

// #include <any>
// #include <stdexcept>
// #include <string>
// #include <concepts>

#include <Usagi/Runtime/Memory/Region.hpp>

namespace usagi
{
class AssetPackage;

enum class AssetPriority : std::uint8_t
{
    // Scripts, collision models, etc. Frame data computation cannot be properly
    // done without presence of them.
    GAMEPLAY_CRITICAL   = 3,

    // Normal streamed assets such as graphics content, detailed textures,
    // fine models, etc. Missing them doesn't affect the computational
    // correctness of data, but will be disturbing.
    STREAMING           = 2,

    // BGM, surrounding details, etc. Doesn't affect gameplay and are generally
    // unnoticeable when loaded slowly, unlike blurry textures or missing
    // model details.
    AMBIENT             = 1,
};

/*
struct AssetNotFound final : std::runtime_error
{
    const std::string_view asset_path;

    explicit AssetNotFound(std::string_view asset_path)
        : runtime_error("Requested asset was not found in any asset package.")
        , asset_path(std::move(asset_path))
    {
    }
};

using AssetFeatureChecksum = std::uint64_t;

template <typename T>
concept AssetBuilder = requires (T t) {
    typename T::OutputT;
    // Cache identifier is a number used together with the processor type
    // to index the processed asset caches. The identifier should reflect the
    // parameters of the processor. For example, for an image decoder, the
    // identifier should uniquely reflect the pixel formal, number of channels,
    // etc. For video decoder, it should be able to index the caches by video
    // chunks so the video could be partially loaded.
    { t.hash() } -> std::same_as<AssetFeatureChecksum>;
    { t.build() } -> std::same_as<MemoryRegion>;
    { T::free(std::declval<const MemoryRegion &>()) };
};
*/

enum class AssetStatus : std::uint64_t
{
    // For primary assets, this means the asset couldn't be found in any source.
    // For secondary assets, this means the asset couldn't be found in the
    // cache. A secondary asset handler must be provided to rebuild the cache.
    MISSING = 0,

    // The asset exists, but the current operation will not cause it to be
    // loaded into memory.
    EXIST   = 1,

    // A task has been queued to load the asset into memory.
    QUEUED  = 2,

    // A task is actively loading the content of asset into memory.
    LOADING = 3,

    // The asset is loaded.
    READY   = 4,

    // A primary dependency could not be found.
    // Although missing a dependency could be a critical error, it is not hard
    // to fix once detected. A status flag provides an opportunity for the
    // caller to decide how to react to the incident. On the other hand,
    // the failure of secondary asset handler in processing the asset usually
    // indicates more serious errors such as bugs. So in that case, it's
    // more suitable to throw an exception.
    MISSING_DEPENDENCY = 5,
};

// Asset fingerprint is used for validate the content of assets. For primary
// assets, it is the hash of the asset content. For secondary assets, it
// reflects the content of primary dependencies and secondary asset build
// parameters.
using AssetFingerprint = std::uint64_t;

struct PrimaryAssetMeta
{
    ReadonlyMemoryRegion region;
    AssetFingerprint fingerprint = 0;
    AssetPackage *package = nullptr;
    AssetStatus status:8 = AssetStatus::MISSING;
    std::uint64_t loading_task_id:56 = -1;
};

struct SecondaryAssetMeta
{
    class SecondaryAsset *asset = nullptr;
    // Secondary asset fingerprint consists of two parts. The build fingerprint
    // is the hash of secondary asset handler type and build parameters, which
    // is used for querying in the cache for the secondary asset. The
    // dependency fingerprint is the hash of fingerprints of primary assets
    // used when building the secondary asset. It can be used to detect outdated
    // secondary cache entries when the content of dependent primary assets
    // changes.
    AssetFingerprint fingerprint_build = 0;
    AssetFingerprint fingerprint_dep_content = 0;
    // AssetPackage *package = nullptr;
    AssetStatus status:8 = AssetStatus::MISSING;
    std::uint64_t loading_task_id:56 = -1;
};
}
