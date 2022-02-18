#pragma once

#include <cassert>

#include <Usagi/Library/Utilities/Functional.hpp>

#include "ResourceHasher.hpp"

namespace usagi::details::heap_manager
{
template <ResourceBuilder ResourceBuilderT, typename ... Args>
HeapResourceDescriptor make_resource_descriptor(Args &&... args)
requires std::constructible_from<ResourceBuilderT, Args...>
{
    // Use the type hash of the builder and build parameters to build a
    // unique id of the requested asset.
    ResourceHasher hasher;
    // The hash code of the builder type can be used to detect collision.
    const auto type_hash = typeid(ResourceBuilderT).hash_code();
    hasher.append(type_hash);
    (..., hasher.append(args));

    const auto hash = hasher.hash();
    assert(hash && "Resource Id shouldn't be zero.");

    return { typeid(ResourceBuilderT).hash_code(), hash };
}

template <ResourceBuilder ResourceBuilderT, typename Tuple>
HeapResourceDescriptor make_resource_descriptor_from_tuple(
    Tuple &&tuple)
{
    return USAGI_APPLY(
        make_resource_descriptor<ResourceBuilderT>,
        tuple
    );
}
}
