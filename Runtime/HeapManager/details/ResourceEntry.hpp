#pragma once

#include <atomic>
#include <future>
#include <typeindex>

#include <Usagi/Library/Utilities/TransparentlyComparable.hpp>
#include <Usagi/Runtime/Memory/RefCount.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceState.hpp"

namespace usagi
{
struct ResourceEntry
    : TransparentlyComparable<ResourceEntry, HeapResourceDescriptor>
{
    HeapResourceDescriptor descriptor;

    mutable RefCounter use_count;
    mutable std::atomic<ResourceState> state = 
        ResourceState::ABSENT_FIRST_REQUEST;

    // std::shared_mutex availability;
    // todo simplify the block behavior
    mutable std::shared_future<void> future;

    explicit ResourceEntry(HeapResourceDescriptor descriptor)
        : descriptor(std::move(descriptor))
    {
    }

    const HeapResourceDescriptor & key() const
    {
        return descriptor;
    }
};
}
