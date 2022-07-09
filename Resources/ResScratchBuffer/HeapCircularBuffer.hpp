#pragma once

#include <map>
#include <Usagi/Library/Meta/Template.hpp>
#include <Usagi/Modules/Runtime/HeapManager/Heap.hpp>
#include <Usagi/Runtime/Memory/VmAllocatorPagefileBacked.hpp>
#include <Usagi/Runtime/Memory/Allocators/CircularAllocator.hpp>

namespace usagi
{
class HeapCircularBuffer : public Heap
{
    VmAllocatorPagefileBacked mMemory;
    CircularAllocator mAllocator;

    // todo shouldn't need this.
    std::map<HeapResourceIdT, std::shared_ptr<void>> mAllocations;

    std::shared_ptr<void> do_allocate(HeapResourceIdT id, std::size_t size);
    std::shared_ptr<void> find_allocation(HeapResourceIdT id);

public:
    explicit HeapCircularBuffer(std::size_t reserved_bytes);

    template <typename SharedPtr>
    SharedPtr allocate(const HeapResourceIdT id, const std::size_t size)
    {
        using TargetElementT = typename SharedPtr::element_type;
        static_assert(std::is_trivially_destructible_v<TargetElementT>);
        const auto memory = do_allocate(id, size);
        return std::static_pointer_cast<       
            typename ExtractFirstTemplateArgument<SharedPtr>::type
        >(memory);
    }

    template <typename SharedPtr>
    SharedPtr resource(const HeapResourceIdT id)
    {
        using TargetElementT = typename SharedPtr::element_type;
        static_assert(std::is_trivially_destructible_v<TargetElementT>);
        const auto memory = find_allocation(id);
        return std::static_pointer_cast<
           typename ExtractFirstTemplateArgument<SharedPtr>::type
        >(memory);
    }
};
}
