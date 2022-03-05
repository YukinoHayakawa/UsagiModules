#pragma once

#include <cassert>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Library/Memory/Nonmovable.hpp>
#include <Usagi/Library/Utilities/Functional.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceAccessor.hpp"
#include "ResourceRequestContext.hpp"

namespace usagi
{
class TaskExecutor;
class HeapManager;

template <typename SrcHeap, typename SrcRes, typename DstHeap, typename DstRes>
struct HeapTransfer;

template <typename Heap, typename Product, typename... Args>
concept HeapHasTemplatedAllocateFunc = requires(Heap h)
{
    h.template allocate<Product>(HeapResourceIdT(), std::declval<Args>()...);
};

template <typename ResourceBuilderT>
class ResourceConstructDelegate : Noncopyable, Nonmovable
{
public:
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ProductT = typename ResourceBuilderT::ProductT;
    using ContextT = ResourceBuildContext<ResourceBuilderT>;

private:
    ContextT *mContext = nullptr;
    bool mObjectAllocated = false;

public:
    explicit ResourceConstructDelegate(ContextT *context);
    ~ResourceConstructDelegate();

    // ********************************************************************* //    
    //                           Resource Allocation                         //
    // ********************************************************************* //

    /*
     * Allocate the resource from the target heap.
     * A resource builder must call this function once and only once during
     * construct(). Failing to do so will be treated as failure in building
     * the resource.
     */
    template <typename... Args>
    decltype(auto) allocate(Args &&...args);

    template <typename ArgTuple>
    decltype(auto) allocate_apply(ArgTuple &&args_tuple);

    // ********************************************************************* //    
    //                            Resource Request                           //
    // ********************************************************************* //

    template <typename AnotherBuilderT, typename... Args>
    [[nodiscard]]
    auto resource(Args &&...build_params)
        -> ResourceAccessor<AnotherBuilderT>;

    template <typename AnotherBuilderT, typename ArgTuple>
    [[nodiscard]]
    auto resource_apply(ArgTuple &&args_tuple)
        -> ResourceAccessor<AnotherBuilderT>;

    // ********************************************************************* //    
    //                           Resource Transfer                           //
    // ********************************************************************* //

    template <
        typename AnotherBuilderT,
        typename... Args
    >
    [[nodiscard]]
    auto transfer(ProductT &dst_res, Args &&...args);
};
}
