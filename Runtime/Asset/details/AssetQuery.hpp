#pragma once

#include <Usagi/Runtime/Memory/View.hpp>

namespace usagi
{
class AssetPackage;

/*
 * AssetQuery is a dispose-after-use object solely used as a bridge to
 * communicate with certain asset package.
 */
class AssetQuery
{
    // protected:
    // std::optional<AssetHashId> mFingerprint;
    //
    // static AssetHashId hash_memory_region(ReadonlyMemoryView region);
    // virtual AssetHashId fingerprint_impl() = 0;

public:
    virtual ~AssetQuery() = default;

    virtual AssetPackage * package() const = 0;

    // Returns whether the content of the specified asset is already loaded
    // into memory.
    // todo: heap manager?
    virtual bool ready() const = 0;

    // Load specified asset pages into memory.
    virtual void fetch() = 0;

    // AssetHashId fingerprint();

    // Get a readonly reference to the memory region.
    virtual ReadonlyMemoryView data() = 0;

    // Remove the asset from memory and invalidate any memory reference
    // to it. The memory is released.
    // virtual void evict() = 0;
};
}
