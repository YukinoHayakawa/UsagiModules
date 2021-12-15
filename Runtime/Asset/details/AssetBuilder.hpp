#pragma once

#include <memory>
#include <type_traits>

#include "../Asset.hpp"

namespace usagi
{
class TaskExecutor;
class AssetManager2;

// using AssetBuildResult = std::pair<AssetStatus, std::unique_ptr<Asset>>;

class AssetBuilder
{
public:
    virtual ~AssetBuilder() = default;

    virtual std::unique_ptr<Asset> construct_with(
        AssetManager2 &asset_manager,
        TaskExecutor &executor) = 0;
};

template <typename T>
concept IsProperAssetBuilder =
    std::is_base_of_v<AssetBuilder, T> &&
    requires(T) { typename T::ProductT; } &&
    std::is_base_of_v<Asset, typename T::ProductT>
;
}
