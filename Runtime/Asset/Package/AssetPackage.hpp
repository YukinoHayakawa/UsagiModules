#pragma once

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Library/Memory/StackPolymorphicObject.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetQuery.hpp>

namespace usagi
{
// Provide access to raw binary data of assets.
class AssetPackage
    : Noncopyable
    , public std::enable_shared_from_this<AssetPackage>
{
public:
    AssetPackage() = default;
    virtual ~AssetPackage() = default;

    // Asset package impl should construct a query object in the provided
    // stack memory, storing the information related to the specified asset
    // to save subsequent asset search time.
    virtual bool create_query(
        std::string_view path,
        StackPolymorphicObject<AssetQuery> &query) = 0;
};
}
