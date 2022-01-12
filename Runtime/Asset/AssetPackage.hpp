#pragma once

#include <Usagi/Library/Memory/MemoryArena.hpp>
#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Modules/Runtime/Asset/details/AssetEnum.hpp>
#include <Usagi/Runtime/ReturnValue.hpp>

#include "details/AssetPath.hpp"

namespace usagi
{
class AssetQuery;
class AssetChangeCallbackProxy;

// Provide access to raw binary data of assets.
class AssetPackage : Noncopyable
{
public:
    AssetPackage() = default;
    virtual ~AssetPackage() = default;

    // Asset package impl should construct a query object in the provided
    // stack memory, storing the information related to the specified asset
    // to save subsequent asset search time.
    virtual ReturnValue<AssetStatus, AssetQuery *> create_query(
        AssetPath path,
        MemoryArena &arena) = 0;

    // Unload asset file from memory.
    virtual bool evict(AssetPath path) = 0;

    virtual void poll_asset_changes(AssetChangeCallbackProxy &callback) = 0;

    virtual std::string_view type() const = 0;
    virtual std::string_view root() const = 0;
};
}
