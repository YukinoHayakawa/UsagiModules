#pragma once

#include <cassert>

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
template <typename HeapT>
HeapT * HeapManager::add_heap(
    const HeapResourceIdT heap_id,
    std::unique_ptr<HeapT> heap)
{
    LOG(trace,
        "[Heap] Adding heap: {:#0x} ({})",
        heap_id,
        typeid(HeapT).name()
    );
    std::unique_lock lk(mHeapMutex);
    auto [it, inserted] = mHeaps.try_emplace(heap_id, std::move(heap));
    assert(inserted);
    return static_cast<HeapT *>(it->second.get());
}

template <typename HeapT>
HeapT * HeapManager::locate_heap(const HeapResourceIdT heap_id)
{
    std::shared_lock lk(mHeapMutex);

    const auto it = mHeaps.find(heap_id);

    USAGI_ASSERT_THROW(
        it != mHeaps.end(),
        std::runtime_error(
            std::format(
                "Heap {:#0x} doesn't exist.", heap_id
            )
        )
    );

    lk.unlock();

    auto ptr = dynamic_cast<HeapT *>(it->second.get());

    USAGI_ASSERT_THROW(
        ptr != nullptr,
        std::runtime_error(std::format(
            "Heap {:#0x} doesn't match with the requested type.", heap_id
        ))
    );

    return ptr;
}
}
