#pragma once

#include <Usagi/Library/Memory/MemoryArena.hpp>
#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "details/AssetPath.hpp"

namespace usagi
{
class AssetManager2;
class AssetQuery;

// Provide access to raw binary data of assets.
class AssetPackage : Noncopyable
{
public:
    AssetPackage() = default;
    virtual ~AssetPackage() = default;

    // Asset package impl should construct a query object in the provided
    // stack memory, storing the information related to the specified asset
    // to save subsequent asset search time.
    virtual AssetQuery * create_query(
        AssetPath path,
        MemoryArena &arena) = 0;

    virtual void report_asset_changes(AssetManager2 &manager) { }

    virtual std::string_view type() const = 0;
    virtual std::string_view root() const = 0;
};
}
