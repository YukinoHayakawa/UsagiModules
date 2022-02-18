#pragma once

#include "HeapResourceDescriptor.hpp"
#include "ResourceEntry.hpp"
#include "ResourceState.hpp"

namespace usagi
{
template <typename ResourceBuilderT>
class ResourceBuildTask;

class ResourceAccessorBase{};

template <typename Heap, typename Product>
concept HeapHasTemplatedResourceFunc = requires(Heap h)
{
    h.template resource<Product>(HeapResourceIdT());
};

/**
 * \brief Use this like a smart pointer!
 * \tparam ResourceBuilderT
 */
template <typename ResourceBuilderT>
class ResourceAccessor
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ResourceT = typename ResourceBuilderT::ProductT;

    template <typename BuilderT>
    friend class ResourceBuildTask;

    template <typename BuilderT>
    friend class ResourceConstructDelegate;

    template <
        typename BuilderT,
        typename BuildParamTupleFunc
    >
    friend struct ResourceRequestHandler;

    HeapResourceDescriptor mDescriptor;
    ResourceEntry *mEntry = nullptr;
    TargetHeapT *mHeap = nullptr;
    ResourceState mStateSnapshot;
    bool mIsFallback = false;

    void fetch_state()
    {
        mStateSnapshot = mEntry->state.load(std::memory_order::acquire);
    }

public:
    ResourceAccessor() = default;

    ResourceAccessor(
        HeapResourceDescriptor descriptor,
        ResourceEntry *entry,
        TargetHeapT *heap,
        ResourceState state_snapshot,
        const bool is_fallback)
        : mDescriptor(std::move(descriptor))
        , mEntry(entry)
        , mHeap(heap)
        , mStateSnapshot(std::move(state_snapshot))
        , mIsFallback(is_fallback)
    {
    }

    // todo Move ops & Do ref cnt

    // todo: make sure the pointer is only available when the accessor is alive. use weak ref
    // todo: some heap may return by value?
    decltype(auto) get()
    {
        fetch_state();
        USAGI_ASSERT_THROW(
            last_state().ready(),
            std::runtime_error("Resource not ready.")
        );
        // Asks the heap for object.
        if constexpr(HeapHasTemplatedResourceFunc<TargetHeapT, ResourceT>)
            return mHeap->template resource<ResourceT>(
                mDescriptor.resource_id());
        else
            return mHeap->resource(
                mDescriptor.resource_id());
    }

    decltype(auto) await()
    {
        // Wait for the future.
        mEntry->future.wait();
        return get();
    }

    ResourceState last_state() const
    {
        return mStateSnapshot;
    }

    HeapResourceDescriptor descriptor() const
    {
        return mDescriptor;
    }

    bool is_fallback() const
    {
        return mIsFallback;
    }
};
}
