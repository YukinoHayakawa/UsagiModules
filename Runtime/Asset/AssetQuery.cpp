#include "AssetQuery.hpp"

#include <cassert>

#include "External/xxhash/xxhash64.h"

namespace usagi
{
AssetFingerprint AssetQuery::hash_memory_region(ReadonlyMemoryRegion region)
{
    assert(region);
    XXHash64 hasher(0);
    hasher.add(region.base_address, region.length);
    return hasher.hash();
}

AssetFingerprint AssetQuery::fingerprint()
{
    if(!mFingerprint) mFingerprint = fingerprint_impl();
    return mFingerprint.value();
}
}
