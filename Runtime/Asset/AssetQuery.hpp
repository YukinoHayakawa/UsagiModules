#pragma once

#include <optional>

#include "Asset.hpp"

namespace usagi
{
class AssetPackage;

class AssetQuery
{
protected:
    std::optional<AssetFingerprint> mFingerprint;

    [[nodiscard]]
    static AssetFingerprint hash_memory_region(ReadonlyMemoryRegion region);

    [[nodiscard]]
    virtual AssetFingerprint fingerprint_impl() = 0;

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

    [[nodiscard]]
    AssetFingerprint fingerprint();

    // Get a readonly reference to the memory region.
    [[nodiscard]]
    virtual ReadonlyMemoryRegion memory_region() = 0;

    // Remove the asset from memory and invalidate any memory reference
    // to it. The memory is released.
    // virtual void evict() = 0;
};
}
