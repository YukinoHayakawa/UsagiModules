#pragma once

#include <Usagi/Runtime/Memory/Region.hpp>

namespace usagi
{
class AssetPackage;

class AssetQuery
{
public:
    virtual ~AssetQuery() = default;

    [[nodiscard]]
    virtual AssetPackage * package() const = 0;

    // Returns whether the content of the specified asset is already loaded
    // into memory.
    // todo: heap manager?
    [[nodiscard]]
    virtual bool prefetched() const = 0;

    // Load specified asset pages into memory.
    virtual void fetch() = 0;

    // Get a readonly reference to the memory region.
    [[nodiscard]]
    virtual ReadonlyMemoryRegion memory_region() = 0;

    // Remove the asset from memory and invalidate any memory reference
    // to it. The memory is released.
    // virtual void evict() = 0;
};
}
