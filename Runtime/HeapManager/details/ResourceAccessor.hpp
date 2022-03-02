#pragma once

#include <Usagi/Library/Meta/Template.hpp>
#include <Usagi/Runtime/Memory/RefCount.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceEntry.hpp"
#include "ResourceState.hpp"

namespace usagi
{
template <typename ResourceBuilderT>
class ResourceBuildTask;

class ResourceAccessorBase{};

namespace details::heap_manager
{
template <typename Heap, typename Product>
concept HeapUsingTemplatedAlloc = requires(Heap h)
{
    h.template resource<Product>(HeapResourceIdT());
};

template <typename ResourceBuilder>
concept TargetHeapUsingTemplatedAlloc =
    HeapUsingTemplatedAlloc<
        typename ResourceBuilder::TargetHeapT,
        typename ResourceBuilder::ProductT
    >;

template <typename ResourceBuilderT>
class HeapResourceReturnT
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;

public:
    using Type = decltype(
        std::declval<TargetHeapT>().resource({ })
    );
};

template <TargetHeapUsingTemplatedAlloc ResourceBuilderT>
class HeapResourceReturnT<ResourceBuilderT>
{
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ResourceT = typename ResourceBuilderT::ProductT;

public:
    using Type = decltype(
        std::declval<TargetHeapT>().template resource<ResourceT>({ })
    );
};

template <typename ResourceBuilderT>
using ResourceReturnT = typename HeapResourceReturnT<ResourceBuilderT>::Type;

template <typename ReturnT>
using DeduceResourceRefCntT =
    std::conditional_t<
        std::is_reference_v<ReturnT>,
        std::remove_reference_t<ReturnT>,   // Convert reference to pointer
        InPlace<ReturnT>                    // Otherwise store the value
    >;

static_assert(std::is_same_v<DeduceResourceRefCntT<int>, InPlace<int>>);
static_assert(std::is_same_v<DeduceResourceRefCntT<int *>, InPlace<int *>>);
static_assert(std::is_same_v<DeduceResourceRefCntT<const int *>, InPlace<const int *>>);
static_assert(std::is_same_v<DeduceResourceRefCntT<int &>, int>);
static_assert(std::is_same_v<DeduceResourceRefCntT<const int &>, const int>);
static_assert(std::is_same_v<DeduceResourceRefCntT<const int &&>, const int>);

template <typename ResourceBuilderT>
using ResourceRefCntT = 
    DeduceResourceRefCntT<ResourceReturnT<ResourceBuilderT>>;
}

/**
 * \brief Use this like a smart pointer!
 * \tparam ResourceBuilderT Builder type.
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

    decltype(auto) request_resource()
    {
        using namespace details::heap_manager;

        const auto rid = mEntry->descriptor.resource_id();

        // Asks the heap for object.
        if constexpr(TargetHeapUsingTemplatedAlloc<ResourceBuilderT>)
            return mHeap->template resource<ResourceT>(rid);
        else
            return mHeap->resource(rid);
    }

    decltype(auto) request_resource_converted()
    {
        decltype(auto) res = request_resource();
        if constexpr(std::is_reference_v<decltype(res)>)
            return &res;
        else
            return res;
    }

    using ResourceRefCntT =
        details::heap_manager::ResourceRefCntT<ResourceBuilderT>;

    const ResourceEntry *mEntry = nullptr;
    // todo maybe should be moved into ResourceEntry
    TargetHeapT *mHeap = nullptr;
    ResourceState mStateSnapshot;
    bool mIsFallback = false;
    // Resource accessor always holds a ref to the resource to prevent the
    // resource being evicted.
    RefCounted<ResourceRefCntT> mObject;

    void fetch_state()
    {
        mStateSnapshot = mEntry->state.load(std::memory_order::acquire);
    }

public:
    ResourceAccessor() = default;

    ResourceAccessor(
        const ResourceEntry *entry,
        TargetHeapT *heap,
        const bool is_fallback)
        : mEntry(entry)
        , mHeap(heap)
        , mStateSnapshot(mEntry->state.load(std::memory_order::acquire))
        , mIsFallback(is_fallback)
        // Increment the counter first to keep alive the record. The object
        // will be injected later.
        , mObject(&mEntry->use_count)
    {
    }

    // todo Move ops & Do ref cnt

    // todo: make sure the pointer is only available when the accessor is alive. use weak ref
    // todo: some heap may return by value
    auto get()
    {
        if(mObject.has_value()) 
            return mObject;

        fetch_state();
        USAGI_ASSERT_THROW(
            last_state().ready(),
            std::runtime_error("Resource not ready.")
        );

        mObject = RefCounted<ResourceRefCntT> {
            &mEntry->use_count,
            request_resource_converted()
        };

        return mObject;
    }

    auto await()
    {
        if(mObject.has_value()) 
            return mObject;

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
        return mEntry->descriptor;
    }

    bool is_fallback() const
    {
        return mIsFallback;
    }
};
}
