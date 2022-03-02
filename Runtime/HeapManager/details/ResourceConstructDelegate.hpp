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
    explicit ResourceConstructDelegate(ContextT *context)
        : mContext(context)
    {
    }

    ~ResourceConstructDelegate()
    {
        USAGI_ASSERT_THROW(
            mObjectAllocated,
            std::runtime_error("Resource builder didn't allocate any object!")
        );
    }

    /*
     * Allocate the resource from the target heap.
     * A resource builder must call this function once and only once during
     * construct(). Failing to do so will be treated as failure in building
     * the resource.
     */
    template <typename... Args>
    decltype(auto) allocate(Args &&...args)
    {
        // todo stronger test?
        assert(!mObjectAllocated);

        if constexpr(HeapHasTemplatedAllocateFunc<
            TargetHeapT, ProductT, Args...>)
        {
            decltype(auto) ret = mContext->heap->template allocate<ProductT>(
                mContext->entry->descriptor.resource_id(),
                std::forward<Args>(args)...
            );
            mObjectAllocated = true;
            return ret;
        }
        else
        {
            decltype(auto) ret = mContext->heap->allocate(
                mContext->entry->descriptor.resource_id(),
                std::forward<Args>(args)...
            );
            mObjectAllocated = true;
            return ret;
        }

        // decltype(auto) ret = mContext->heap->allocate(
        //     Tag<ProductT>(),
        //     mContext->entry->descriptor.resource_id(),
        //     std::forward<Args>(args)...
        // );
        // mObjectAllocated = true;
        //
        // return ret;
    }

    template <typename ArgTuple>
    decltype(auto) allocate_apply(ArgTuple &&args_tuple)
    {
        return USAGI_APPLY(allocate, args_tuple);
    }

    template <
        typename AnotherBuilderT,
        typename... Args
    >
    [[nodiscard]]
    auto resource(Args &&...build_params)
        -> ResourceAccessor<AnotherBuilderT>;

    template <
        typename AnotherBuilderT,
        typename... Args
    >
    [[nodiscard]]
    auto transfer(ProductT &dst_res, Args &&...args)
    {
        auto src_accessor = resource<AnotherBuilderT>(
            std::forward<Args>(args)...
        );
        auto src_heap = src_accessor.mHeap;
        decltype(auto) src_res = src_accessor.await();

        // the transfer function may interact with the heaps to accomplish the
        // operation. e.g. gpu copy
        HeapTransfer<
            typename AnotherBuilderT::TargetHeapT,
            typename AnotherBuilderT::ProductT,
            TargetHeapT,
            ProductT
        > transfer_func;

        return transfer_func(
            *src_heap,
            src_res.cref(),
            *mContext->heap,
            dst_res
        );
    }
};
}
