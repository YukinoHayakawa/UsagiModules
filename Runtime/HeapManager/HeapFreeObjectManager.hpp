#pragma once

#include <cassert>
#include <map>

#include "details/HeapResourceDescriptor.hpp"

#include "Heap.hpp"
#include "Resource.hpp"

namespace usagi
{
class HeapFreeObjectManager : public Heap
{
    std::map<std::uint64_t, std::unique_ptr<Resource>> mObjects;

public:
    // The heap is free to provide whatever kind of service to builders.
    // As long as the managed objects can be accessed via ids.
    template <typename T, typename... Args>
    T & allocate(const HeapResourceIdT id, Args &&...args)
    {
        auto [it, inserted] = mObjects.try_emplace(
            id,
            std::make_unique<T>(std::forward<Args>(args)...)
        );
        assert(inserted);
        return static_cast<T &>(*it->second);
    }

    template <typename T>
    const T & resource(const HeapResourceIdT id)
    {
        const auto it = mObjects.find(id);
        assert(it != mObjects.end());
        T *ptr = dynamic_cast<T *>(it->second.get());
        assert(ptr);
        return *ptr;
    }
};
}
