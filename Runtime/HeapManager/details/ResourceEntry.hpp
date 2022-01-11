#pragma once

#include <atomic>
#include <future>
#include <typeindex>

#include "HeapResourceDescriptor.hpp"
#include "ResourceState.hpp"

namespace usagi
{
struct ResourceEntry
{
    // HeapResourceDescriptor descriptor;
    std::atomic<std::uint64_t> num_refs = 0;
    std::type_index builder_type = typeid(void);
    std::atomic<ResourceState> state = ResourceState::ABSENT_FIRST_REQUEST;
    std::shared_future<void> future;

    bool can_unload() const;
    bool is_ready() const;
    bool is_failed() const;
    bool is_constructing() const;
    bool is_present() const; // building / ready

    // state.load(std::memory_order::acquire).is_resource_allocated()
};
}
