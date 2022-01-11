#pragma once

#include "HeapResourceDescriptor.hpp"
#include "ResourceEntry.hpp"
#include "ResourceState.hpp"

namespace usagi
{
template <typename ResourceBuilderT>
class ResourceBuildTask;

class ResourceAccessorBase{};

template <typename ResourceBuilderT>
class ResourceAccessor
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ResourceT = typename ResourceBuilderT::ProductT;

    friend class HeapManager;

    template <typename BuilderT>
    friend class ResourceBuildTask;

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

    // todo: make sure the pointer is only available when the accessor is alive
    ResourceT * resource()
    {
        // Asks the heap for object.
        return &mHeap->template resource<ResourceT>(mDescriptor.resource_id());
    }

    ResourceT * await_resource()
    {
        // Wait for the future.
        mEntry->future.wait();
        return resource();
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
