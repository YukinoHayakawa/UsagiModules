#pragma once

#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

#include "HeapCircularBuffer.hpp"

namespace usagi
{
/**
 * \brief Create a buffer that holds data supposed to be disposed soon after
 * use.
 * \tparam Elem Type of elements.
 */
template <typename Elem>
class RbScratchBuffer
{
public:
    using BuildArguments = std::tuple<
        // unique id used to identify each allocation
        HeapResourceDescriptor,
        // number of elements
        TransparentArg<std::size_t>
    >;
    using ProductT = std::shared_ptr<Elem[]>;

    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        HeapResourceDescriptor /* unused */,
        std::size_t num_elems) const
    {
        auto heap = delegate.template heap<HeapCircularBuffer>();
        delegate.emplace(heap->template allocate<Elem>(num_elems));

        return ResourceState::READY;
    }
};
}
