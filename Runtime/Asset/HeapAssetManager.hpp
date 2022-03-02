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
    ReturnValue<AssetStatus, AssetQuery *> allocate(

        HeapResourceIdT id,
        AssetPath path,
        MemoryArena &arena);

    ReadonlyMemoryView resource(HeapResourceIdT id);
};
}
