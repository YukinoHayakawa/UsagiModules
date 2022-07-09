#pragma once

#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/Memory/RefCount.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceEntry.hpp"
#include "ResourceState.hpp"
#include "../ResourceExceptions.hpp"

namespace usagi
{
template <typename ResourceBuilderT>
class ResourceBuildTask;

/**
 * \brief Use this like a smart pointer!
 * \tparam Product Builder type.
 * todo builder type is not needed. use resource type.
 */
template <typename Product>
class ResourceAccessor
{
    template <typename BuilderT>
    friend class ResourceBuildTask;

    template <typename BuilderT>
    friend class ResourceConstructDelegate;

    template <
        typename BuilderT,
        typename BuildParamTupleFunc
    >
    friend struct ResourceRequestHandler;

    ResourceEntry<Product> *mEntry = nullptr;
    ResourceState mStateSnapshot;
    bool mIsFallback = false;
    // Resource accessor always holds a ref to the resource to prevent the
    // resource being evicted.
    RefCounted<Product> mObject;

    void fetch_state()
    {
        mStateSnapshot = mEntry->state.load(std::memory_order::acquire);
    }

public:
    ResourceAccessor() = default;

    ResourceAccessor(
        ResourceEntry<Product> *entry,
        const bool is_fallback)
        : mEntry(entry)
        , mStateSnapshot(mEntry->state.load(std::memory_order::acquire))
        , mIsFallback(is_fallback)
        // Increment the counter first to keep alive the record. The object
        // will be injected later.
        , mObject(&mEntry->use_count)
    {
    }

    // todo Move ops & Do ref cnt

    // todo: make sure the pointer is only available when the accessor is alive. use weak ref?
    RefCounted<Product> get()
    {
        // return value if already fetched
        if(mObject.has_value()) 
            return mObject;

        // otherwise update the resource state first
        fetch_state();
        USAGI_ASSERT_THROW(
            last_state().ready(),
            ResourceNotReady()
        );

        // if the resource is ready, fetch it
        mObject = RefCounted<Product> {
            &mEntry->use_count,
            &mEntry->payload.value()
        };

        return mObject;
    }

    RefCounted<Product> await()
    {
        if(mObject.has_value()) 
            return mObject;

        // Wait for the future.
        mEntry->future.wait();
        return get();
    }

    bool ready() const
    {
        return last_state().ready();
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
