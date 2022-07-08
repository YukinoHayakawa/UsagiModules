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

/*
template <typename Heap, typename Product, typename... Args>
concept HeapHasTemplatedAllocateFunc = requires(Heap h)
{
    h.template allocate<Product>(HeapResourceIdT(), std::declval<Args>()...);
};
*/

template <typename Product>
class ResourceConstructDelegate : Noncopyable, Nonmovable
{
public:
    // using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    // using ProductT = typename ProductT::ProductT;
    using ProductT = Product;
    using ContextT = ResourceBuildContext<ProductT>;

private:
    ContextT *mContext = nullptr;
    // bool mObjectAllocated = false;

public:
    explicit ResourceConstructDelegate(ContextT *context);
    ~ResourceConstructDelegate();

    // ********************************************************************* //    
    //                           Resource Allocation                         //
    // ********************************************************************* //

    // /*
    //  * Allocate the resource from the target heap.
    //  * A resource builder must call this function once and only once during
    //  * construct(). Failing to do so will be treated as failure in building
    //  * the resource.
    //  */
    // template <typename... Args>
    // [[deprecated]]
    // decltype(auto) allocate(Args &&...args);
    //
    // /*
    //  * Create the target resource on the target heap with the given
    //  * tuple of arguments.
    //  */
    // template <typename ArgTuple>
    // [[deprecated]]
    // decltype(auto) allocate_apply(ArgTuple &&args_tuple);

    void emplace(ProductT product, std::function<void()> deleter = []{});

    template <typename HeapT>
    HeapT * heap();

    // ********************************************************************* //    
    //                            Resource Request                           //
    // ********************************************************************* //

    template <typename AnotherBuilderT, typename... Args>
    [[nodiscard]]
    auto resource(Args &&...build_params)
        -> ResourceAccessor<AnotherBuilderT>;

    template <typename AnotherBuilderT, typename... Args>
    [[nodiscard]]
    auto resource_transient(Args &&...build_params)
        -> ResourceAccessor<AnotherBuilderT>;

    template <
        typename AnotherBuilderT,
        bool Transient = false,
        typename ArgTuple
    >
    [[nodiscard]]
    auto resource_apply(ArgTuple &&args_tuple)
        -> ResourceAccessor<AnotherBuilderT>;

    // request a descriptor that is unique from heap manager. used for
    // requesting transient resources.
    HeapResourceDescriptor make_unique_descriptor() const;

    // ********************************************************************* //    
    //                           Resource Transfer                           //
    // ********************************************************************* //

    /*
    template <
        typename AnotherBuilderT,
        typename... Args
    >
    [[nodiscard]]
    auto transfer(ProductT &dst_res, Args &&...args);
    */
};
}
