#include "HeapCircularBuffer.hpp"

namespace usagi
{
HeapCircularBuffer::HeapCircularBuffer(const std::size_t reserved_bytes)
    : mMemory(reserved_bytes)
{
    // allocate the memory before attach the allocator to it
    mMemory.allocate(mMemory.reserved_bytes());
    mAllocator = CircularAllocator(
        { mMemory.base_address(), mMemory.reserved_bytes() }
    );
}

std::shared_ptr<void> HeapCircularBuffer::do_allocate(std::size_t size)
{
    return std::shared_ptr<void>(
        mAllocator.allocate(size),
        [this](void *ptr) { mAllocator.deallocate(ptr); }
    );
}
}
