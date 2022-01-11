#pragma once

#include <cstdint>

namespace usagi
{
using HeapResourceIdT = std::uint64_t;

struct HeapResourceDescriptor
{
    HeapResourceIdT heap_id_ { };
    HeapResourceIdT build_option_hash_ { };

    HeapResourceIdT heap_id() const { return heap_id_; }
    HeapResourceIdT resource_id() const { return build_option_hash_; }

    operator bool() const
    {
        return heap_id_ && build_option_hash_;
    }
};
}
