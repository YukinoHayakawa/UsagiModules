#pragma once

#include <any>
#include <stdexcept>
#include <string>
#include <concepts>

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

enum class AssetStatus : std::uint64_t
{
    // The asset couldn't be found in any source.
    PRIMARY_MISSING,

    // The primary asset exists, but no party has shown intend of loading it
    // into memory.
    PRIMARY_FOUND,

    // The primary asset exists, there is a job loading it into memory created,
    // but the job is not running.
    PRIMARY_PENDING,

    // A job is actively loading the content of asset into memory.
    PRIMARY_LOADING,

    // The primary asset is loaded.
    PRIMARY_READY,

    // The secondary asset could not be found in the cache. A constructor
    // must be provided to query its status.
    SECONDARY_MISSING,

    // The secondary asset is in the work queue to be processed.
    SECONDARY_PENDING,

    // The secondary asset with provided processing parameters is still
    // being processed.
    SECONDARY_PROCESSING,

    // The secondary asset with provided processing parameters is in the cache
    // for use.
    SECONDARY_READY,
};

struct PrimaryAsset
{
    ReadonlyMemoryRegion region;
    AssetPackage *package = nullptr;
    AssetStatus status:8 = AssetStatus::PRIMARY_MISSING;
    std::uint64_t loading_task_id:56 = -1;
};

struct AssetCacheSignature
{
    std::uint64_t a, b;

    operator bool() const
    {
        return a && b;
    }

    friend bool operator<(
        const AssetCacheSignature &lhs,
        const AssetCacheSignature &rhs)
    {
        return std::tie(lhs.a, lhs.b) < std::tie(rhs.a, rhs.b);
    }
};

struct SecondaryAsset
{
    std::any object;
    AssetCacheSignature signature;
    AssetPackage *package = nullptr;
    AssetStatus status:8 = AssetStatus::SECONDARY_MISSING;
    std::uint64_t loading_task_id:56 = -1;
};
}
