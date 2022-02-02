#pragma once

#include <cassert>
#include <map>
#include <shared_mutex>

#include <Usagi/Modules/Runtime/HeapManager/details/HeapResourceDescriptor.hpp>

namespace usagi
{
template <typename ObjectT>
class VulkanObjectManager
{
protected:
    std::shared_mutex mMutex;
    std::map<HeapResourceIdT, ObjectT> mObjects;

    template <typename... Args>
    auto & allocate(const HeapResourceIdT id, Args &&...args)
    {
        std::unique_lock lk(mMutex);
        auto [it, inserted] = mObjects.try_emplace(
            id,
            std::forward<Args>(args)...
        );
        assert(inserted);
        return it->second;
    }

    const auto & resource(const HeapResourceIdT id)
    {
        std::shared_lock lk(mMutex);
        const auto it = mObjects.find(id);
        assert(it != mObjects.end());
        return it->second;
    }
};
}
