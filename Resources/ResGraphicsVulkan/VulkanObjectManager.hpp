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

    template <typename Object, typename... Args>
    auto & allocate_impl(const HeapResourceIdT id, Args &&...args)
        requires std::is_same_v<Object, ObjectT>
    {
        std::unique_lock lk(mMutex);
        auto [it, inserted] = mObjects.try_emplace(
            id,
            std::forward<Args>(args)...
        );
        assert(inserted);
        return it->second;
    }

    template <typename Object>
    auto & resource_impl(const HeapResourceIdT id)
        requires std::is_same_v<Object, ObjectT>
    {
        std::shared_lock lk(mMutex);
        const auto it = mObjects.find(id);
        assert(it != mObjects.end());
        return it->second;
    }
};
}
