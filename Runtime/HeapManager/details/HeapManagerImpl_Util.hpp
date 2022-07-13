#pragma once

#include <cassert>
#include <utility>
#include <type_traits>

#include <Usagi/Library/Utilities/Functional.hpp>

#include "ResourceHasher.hpp"
#include "ResourceBuilderTraits.hpp"

namespace usagi::details::heap_manager
{
template <ResourceBuilder ResourceBuilderT, typename ... Args, std::size_t... I>
HeapResourceDescriptor make_resource_descriptor(
    std::index_sequence<I...>,
    Args &&... args)
{
    // Use the type hash of the builder and build parameters to build a
    // unique id of the requested asset.
    ResourceHasher hasher;
    // The hash code of the builder type can be used to detect collision.
    const auto type_hash = typeid(ResourceBuilderT).hash_code();
    hasher.append(type_hash);
    // note that we may be processing rvalue refs. the hasher should never
    // move from the values passed to it.
    (
        hasher.append(
            hash_forward_or_convert<ResourceBuilderT, I>(
                std::forward<Args>(args)
            )
        ),
        ...
    );

    const auto hash = hasher.hash();
    assert(hash && "Resource Id shouldn't be zero.");

    return { typeid(ResourceBuilderT).hash_code(), hash };
}

// todo if two tuples have a different set of value types but will result in identical argument values passed to the builder due to implicit conversion, two resource objects may be created where there should only be one. - seems solved?
template <ResourceBuilder ResourceBuilderT, typename Tuple>
HeapResourceDescriptor make_resource_descriptor_from_tuple(Tuple &&tuple)
{
    return std::apply([&]<typename... Args>(Args &&...args) {
        return make_resource_descriptor<ResourceBuilderT>(
            std::make_index_sequence<
                std::tuple_size_v<std::remove_reference_t<Tuple>>
            >(),
            std::forward<Args>(args)...
        );
    }, tuple);
}
}
