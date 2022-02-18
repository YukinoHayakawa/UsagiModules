﻿#pragma once

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
template <
    ResourceBuilder ResourceBuilderT,
    typename TargetHeapT
>
auto HeapManager::make_accessor_nolock(
    const HeapResourceDescriptor descriptor,
    TargetHeapT *heap,
    const bool is_fallback)
-> ResourceAccessor<ResourceBuilderT>
{
    assert(static_cast<bool>(descriptor));

    ResourceEntryIt it;

    if(!is_fallback)
    {
        bool inserted = false;
        std::tie(it, inserted) = mResourceEntries.try_emplace(descriptor);
        if(inserted)
        {
            LOG(trace,
                "[Heap] New resource added: {} (builder={}, resource={})",
                descriptor,
                typeid(ResourceBuilderT).name(),
                typeid(typename ResourceBuilderT::ProductT).name()
            );
        }
    }
    else
    {
        it = mResourceEntries.find(descriptor);
    }

    LOG(trace,
        "[Heap] Creating resource accessor (fallback={}): {}",
        is_fallback,
        descriptor
    );

    // A fallback should always exist.
    USAGI_ASSERT_THROW(
        it != mResourceEntries.end(),
        std::runtime_error(
            std::format(
                "Requested resource not found."
            )
        )
    );

    // Fetch the state of the resource. Only critical sections can
    // reset resource state from FAILED->PREPARING,
    // READY->ABSENT_EVICTED. So long as the resource is not in
    // an absent state, it's safe to assume that it is either ready,
    // failed, or has an active build task.
    // The accessor will increase the refcnt of the resource.
    // Therefore, as long as the accessor is alive, the resource should
    // always be in ready state.
    return ResourceAccessor<ResourceBuilderT>(
        descriptor,
        &it->second,
        heap,
        it->second.state.load(std::memory_order::acquire),
        is_fallback
    );
}
}
