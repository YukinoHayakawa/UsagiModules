#include "HeapCircularBuffer.hpp"

#include <cassert>

namespace usagi
{
HeapCircularBuffer::HeapCircularBuffer(std::size_t reserved_bytes)
    : mMemory(reserved_bytes)
{
    // allocate the memory before attach the allocator to it
    mMemory.allocate(mMemory.reserved_bytes());
    mAllocator = CircularAllocator(
        { mMemory.base_address(), mMemory.reserved_bytes() }
    );
}

std::shared_ptr<void> HeapCircularBuffer::do_allocate(
    HeapResourceIdT id,
    std::size_t size)
{
    auto [it, inserted] = mAllocations.try_emplace(id);
    assert(inserted);
    it->second = std::shared_ptr<void>(
        mAllocator.allocate(size),
        [this](void *ptr) { mAllocator.deallocate(ptr); }
    );
    return it->second;
}

std::shared_ptr<void> HeapCircularBuffer::find_allocation(HeapResourceIdT id)
{
    const auto it = mAllocations.find(id);
    assert(it != mAllocations.end());
    return it->second;
}
}
