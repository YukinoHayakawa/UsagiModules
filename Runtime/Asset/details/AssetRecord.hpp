#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <typeindex>

#include "AssetDefs.hpp"

namespace usagi
{
class Asset;

struct AssetRecord
{
    std::unique_ptr<Asset> asset;
    // Holds nothing but a control block to track the usages of this asset
    // todo pool the control block allocation
    AssetReference ref_tracker = std::shared_ptr<void>(nullptr, [](auto) { });
    // Note: future object should be reset when the asset is unloaded.
    std::shared_future<void> future;
    std::type_index builder_type = typeid(void);
    // This variable is used as a synchronization barrier to make sure
    // that once the asset is constructed, it is visible when being
    // requested.
    std::atomic<AssetStatus> status = AssetStatus::MISSING;

    // Allow try_emplace to work
    AssetRecord() = default;
};
}
