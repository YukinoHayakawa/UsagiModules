#pragma once

#include <cassert>

#include "ResourceHasher.hpp"

namespace usagi
{
template <ResourceBuilder ResourceBuilderT, typename ... Args>
HeapResourceIdT HeapManager::make_resource_id(Args &&... args)
{
    // Use the type hash of the builder and build parameters to build a
    // unique id of the requested asset.
    ResourceHasher hasher;
    // The hash code of the builder type can be used to detect collision.
    const auto type_hash = typeid(ResourceBuilderT).hash_code();
    hasher.append(type_hash);
    (..., hasher.append(args));
    assert(hasher.hash() && "Resource Id shouldn't be zero.");
    return hasher.hash();
}
}
