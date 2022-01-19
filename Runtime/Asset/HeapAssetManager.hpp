#pragma once

#include <Usagi/Modules/Runtime/HeapManager/Heap.hpp>
#include <Usagi/Modules/Runtime/HeapManager/details/HeapResourceDescriptor.hpp>
#include <Usagi/Runtime/Memory/View.hpp>

#include "AssetManager2.hpp"
#include "details/AssetQuery.hpp"

namespace usagi
{
class HeapAssetManager : public Heap, public AssetManager2
{
    struct RawAssetEntry
    {
        std::string path;
        AssetPackage *package = nullptr;
        // ReadonlyMemoryView view;
    };
    std::map<HeapResourceIdT, RawAssetEntry> mEntries;

public:
    // todo this should be the first time the asset is queried.
    template <std::same_as<ReadonlyMemoryView> T>
    ReturnValue<AssetStatus, AssetQuery *> allocate(
        HeapResourceIdT id,
        AssetPath path,
        MemoryArena &arena)
    {
        // todo don't manage dependencies
        auto query = create_asset_query(
            0, path, arena
        );

        if(query.code() == AssetStatus::EXIST)
        {
            auto [it, inserted] = mEntries.try_emplace(id);
            it->second.path = path.reconstructed();
            it->second.package = query.value()->package();
            assert(inserted);
        }

        return query;
    }

    template <std::same_as<ReadonlyMemoryView> T>
    T resource(const HeapResourceIdT id)
    {
        // todo has to record loaded assets
        auto it = mEntries.find(id);
        assert(it != mEntries.end());
        MemoryArena arena;
        auto query = it->second.package->create_query(it->second.path, arena);
        assert(query.value());
        return query.value()->data();
    }
};
}
