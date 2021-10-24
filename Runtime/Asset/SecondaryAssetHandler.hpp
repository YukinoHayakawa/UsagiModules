#pragma once

#include <memory>
#include <span>
#include <optional>

#include "Asset.hpp"

namespace usagi
{
class SecondaryAssetHandlerBase
{
public:
    virtual ~SecondaryAssetHandlerBase() = default;

    // todo feed hasher
    virtual AssetCacheSignature signature() = 0;

    // AssetManager will call this function to collect the primary assets
    // required during building the secondary asset. The function will be
    // called multiple times with `index` increasing sequentially from 0,
    // until that `has_value()` of the return value evaluated to false.
    // Empty string_view could be returned and will be ignored. This allows
    // optional dependencies depending on user parameters.
    virtual std::optional<std::string_view> primary_dependencies(
        std::size_t index) = 0;

    // Dependent primary assets will be passed in in the order declared by
    // `primary_dependencies`.
    virtual std::unique_ptr<SecondaryAsset> construct(
        std::span<std::optional<PrimaryAssetMeta>> primary_assets) = 0;
};

template <typename SecondaryAssetType>
class SecondaryAssetHandler : public SecondaryAssetHandlerBase
{
public:
    using SecondaryAssetT = SecondaryAssetType;
};
}
