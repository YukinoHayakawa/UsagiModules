#pragma once

#include <cassert>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Library/Memory/Nonmovable.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceAccessor.hpp"

namespace usagi
{
class TaskExecutor;
class HeapManager;

template <typename SrcHeap, typename SrcRes, typename DstHeap, typename DstRes>
struct HeapTransfer;

template <typename ResourceBuilderT>
class ResourceConstructDelegate : Noncopyable, Nonmovable
{
public:
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ProductT = typename ResourceBuilderT::ProductT;

private:
    HeapResourceDescriptor mDescriptor;
    HeapManager *mManager = nullptr;
    TargetHeapT *mHeap = nullptr;
    TaskExecutor *mExecutor = nullptr;
    bool mObjectAllocated = false;

public:
    ResourceConstructDelegate(
        HeapResourceDescriptor descriptor,
        HeapManager *manager,
        TargetHeapT *heap,
        TaskExecutor *executor)
        : mDescriptor(std::move(descriptor))
        , mManager(manager)
        , mHeap(heap)
        , mExecutor(executor)
    {
    }

    ~ResourceConstructDelegate()
    {
        USAGI_ASSERT_THROW(
            mObjectAllocated,
            std::runtime_error("Resource builder didn't allocate any object!")
        );
    }

    // Builder must call this allocate function because it should only create
    // the corresponding object in the target heap.
    template <typename... Args>
    decltype(auto) allocate(Args &&...args)
    {
        assert(!mObjectAllocated);
        mObjectAllocated = true;
        return mHeap->template allocate<ProductT>(
            mDescriptor.resource_id(),
            std::forward<Args>(args)...
        );
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
        return transfer_func(*src_heap, src_res, *mHeap, dst_res);
    }
};
}
