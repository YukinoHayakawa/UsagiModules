#pragma once

#include <cstdint>

#include <fmt/format.h>

namespace usagi
{
using HeapResourceIdT = std::uint64_t;

class HeapResourceDescriptor
{
    HeapResourceIdT mHeapTypeHash { };
    HeapResourceIdT mBuildParamHash { };

public:
    HeapResourceDescriptor() = default;

    HeapResourceDescriptor(
        HeapResourceIdT heap_type_hash,
        HeapResourceIdT build_param_hash)
        : mHeapTypeHash(heap_type_hash)
        , mBuildParamHash(build_param_hash)
    {
    }

    HeapResourceIdT heap_id() const { return mHeapTypeHash; }
    HeapResourceIdT resource_id() const { return mBuildParamHash; }

    explicit operator bool() const
    {
        return mHeapTypeHash && mBuildParamHash;
    }

    auto operator<=>(const HeapResourceDescriptor &rhs) const = default;
};
}

template <>
struct fmt::formatter<usagi::HeapResourceDescriptor>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(
        const usagi::HeapResourceDescriptor &d,
        FormatContext &ctx)
    -> decltype(ctx.out())
    {
        // ctx.out() is an output iterator to write to.
        return format_to(
            ctx.out(),
            "[{:#0x}|{:#016x}]",
            d.heap_id(),
            d.resource_id()
        );
    }
};
