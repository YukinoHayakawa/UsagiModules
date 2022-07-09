#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/Runtime/HeapManager/TransparentArg.hpp>

#include "HeapCircularBuffer.hpp"

namespace usagi
{
/**
 * \brief Create a buffer that holds data supposed to be disposed soon after
 * use.
 * \tparam Elem Type of elements.
 */
template <typename Elem>
class RbScratchBuffer : ArgumentStorage<
    HeapResourceDescriptor,         // temp id, needed to keep each allocation
                                    // unique.
    TransparentArg<std::size_t>     // num elems
>
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapCircularBuffer;
    using ProductT = std::shared_ptr<Elem[]>;

    ResourceState construct(
        ResourceConstructDelegate<RbScratchBuffer> &delegate)
    {
        delegate.allocate(sizeof(Elem) * arg<1>());

        return ResourceState::READY;
    }
};
}
