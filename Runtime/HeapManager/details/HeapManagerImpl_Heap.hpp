#pragma once

#include <cassert>
#include <format>

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
template <typename HeapT, typename... Args>
HeapT * HeapManager::add_heap(Args &&...args)
{
    return add_heap<HeapT>(std::make_unique<HeapT>(std::forward<Args>(args)...));
}

template <typename HeapT>
HeapT * HeapManager::add_heap(std::unique_ptr<HeapT> heap)
{
    LOG(trace, "[Heap] Adding heap: {}", typeid(HeapT).name());

    std::unique_lock lk(mHeapMutex);
    auto [it, inserted] = mHeaps.try_emplace(
        typeid(HeapT),
        std::shared_ptr { std::move(heap) }
    );
    assert(inserted);

    return std::any_cast<std::shared_ptr<HeapT>>(it->second).get();
}

template <typename HeapT>
HeapT * HeapManager::locate_heap()
{
    std::shared_lock lk(mHeapMutex);

    const auto &heap_id = typeid(HeapT); //.hash_code();
    const auto it = mHeaps.find(heap_id);

    USAGI_ASSERT_THROW(
        it != mHeaps.end(),
        std::runtime_error(
            std::format(
                "Heap {} doesn't exist.", heap_id.name()
            )
        )
    );

    lk.unlock();

    // auto ptr =
    return std::any_cast<std::shared_ptr<HeapT>>(it->second).get();

    // USAGI_ASSERT_THROW(
        // ptr != nullptr,
        // std::runtime_error(std::format(
            // "Heap {:#0x} doesn't match with the requested type.", heap_id
        // ))
    // );

    // return ptr;
}
}
