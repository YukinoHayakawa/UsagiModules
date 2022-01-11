#pragma once

#include <cstddef>

namespace usagi
{
/*
 * A heap manages some region of memory. It could be a consecutive range of
 * virtual memory addresses, a device-local heap on GPU manipulated via
 * graphics APIs, a delegated allocator which redirects allocation requests
 * to language runtime, etc.
 * A heap is responsible for managing the lifetime of objects allocated from
 * it. If it decides that an object shall be released, it must notify the
 * HeapManager to update the corresponding resource state.
 */
class Heap
{
public:
    virtual ~Heap() = default;

    /*
     *
     */
    // template <typename ObjectT>
    // auto allocate(std::size_t size, MemoryPriority priority)
    // {
    //
    // }
    //
    // auto deallocate();
    //
    // // Number of bytes that can be allocated.
    // virtual std::size_t available() const = 0;
    // // Number of bytes already allocated.
    // virtual std::size_t used() const = 0;
    // // Total number of bytes owned by the heap.
    // virtual std::size_t capacity() const = 0;

};
}
