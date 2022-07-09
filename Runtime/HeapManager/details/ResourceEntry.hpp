#pragma once

#include <atomic>
#include <future>

#include <Usagi/Runtime/Memory/RefCount.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceState.hpp"

namespace usagi
{
/**
 * \brief Stores the metadata of the resource. The resource objects are stored
 * in derived types. The resource builder is responsible for constructing the
 * resource and provide the deleter, which will be called when the resource
 * is being evicted. 
 */
struct ResourceEntryBase : Noncopyable, Nonmovable
    // : TransparentlyComparable<ResourceEntryBase, HeapResourceDescriptor>
{
    const HeapResourceDescriptor descriptor;

    RefCounter use_count;
    std::atomic<ResourceState> state = ResourceState::ABSENT_FIRST_REQUEST;
    // deletes the resource objects when called.
    // todo deleter never really called
    std::function<void()> deleter;

    // std::shared_mutex availability;
    // todo simplify the blocking behavior
    std::shared_future<void> future;

    // todo no data available to perform deallocation. track the heap?

    explicit ResourceEntryBase(HeapResourceDescriptor descriptor)
        : descriptor(std::move(descriptor))
    {
    }

    virtual ~ResourceEntryBase()
    {
        if(deleter) deleter();
    }

    /*
    const HeapResourceDescriptor & key() const
    {
        return descriptor;
    }*/
};

/**
 * \brief Store the actual resource object.
 * \tparam Resource Resource type.
 */
template <typename Resource> // , typename Deleter>
struct ResourceEntry final : ResourceEntryBase
{
    std::optional<Resource> payload;

    using ResourceEntryBase::ResourceEntryBase;

    ~ResourceEntry() override
    {
        if(deleter) assert(payload.has_value());
    }
};
}
