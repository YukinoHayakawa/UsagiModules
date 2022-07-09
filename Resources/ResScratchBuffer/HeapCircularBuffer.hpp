#pragma once

#include <memory>

#include <Usagi/Runtime/Memory/VmAllocatorPagefileBacked.hpp>
#include <Usagi/Runtime/Memory/Allocators/CircularAllocator.hpp>

namespace usagi
{
class HeapCircularBuffer
{
    VmAllocatorPagefileBacked mMemory;
    CircularAllocator mAllocator;

    std::shared_ptr<void> do_allocate(std::size_t size);

public:
    explicit HeapCircularBuffer(std::size_t reserved_bytes);

    template <typename Elem>
    std::shared_ptr<Elem[]> allocate(const std::size_t num_elems)
        requires std::is_trivially_destructible_v<Elem>
    {
        const auto memory = do_allocate(num_elems * sizeof(Elem));
        return std::static_pointer_cast<Elem[]>(memory);
    }
};
}
