#pragma once

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
template <typename Product>
auto HeapManager::make_accessor_nolock(
    const HeapResourceDescriptor descriptor,
    const bool is_fallback)
-> ResourceAccessor<Product>
{
    assert(static_cast<bool>(descriptor));

    using ProductT = Product;
    using EntryT = ResourceEntry<ProductT>;

    // find existing record
    ResourceEntryIt it = mResourceEntries.lower_bound(descriptor);

    // do not create record when requesting fallback resource
    if(const bool found = it != mResourceEntries.end() && 
        it->get()->descriptor == descriptor; !found && !is_fallback)
    {
        // todo use memory pool to manage resource entries
        auto entry = std::make_unique<EntryT>(descriptor);

        it = mResourceEntries.emplace_hint(
            it,
            std::move(entry)
        );

        LOG(trace,
            "[Heap] Resource added: {} (type={})",
            descriptor,
            typeid(ProductT).name()
        );
    }
    
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
    return ResourceAccessor<ProductT>(
        static_cast<EntryT *>(it->get()),
        is_fallback
    );
}
}
