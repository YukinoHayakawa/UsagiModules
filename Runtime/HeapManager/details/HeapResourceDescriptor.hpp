#pragma once

#include <cstdint>

#include <fmt/format.h>

namespace usagi
{
using HeapResourceIdT = std::uint64_t;

class HeapResourceDescriptor
{
    HeapResourceIdT mHeapId { };
    HeapResourceIdT mBuildParamHash { };

public:
    HeapResourceDescriptor() = default;

    HeapResourceDescriptor(
        HeapResourceIdT heap_id,
        HeapResourceIdT build_param_hash)
        : mHeapId(heap_id)
        , mBuildParamHash(build_param_hash)
    {
    }

    HeapResourceIdT heap_id() const { return mHeapId; }
    HeapResourceIdT resource_id() const { return mBuildParamHash; }

    operator bool() const
    {
        return mHeapId && mBuildParamHash;
    }

    friend auto operator<=>(
        const HeapResourceDescriptor &lhs,
        const HeapResourceDescriptor &rhs)
    {
        return std::tie(lhs.mHeapId, lhs.mBuildParamHash) <=>
            std::tie(rhs.mHeapId, rhs.mBuildParamHash);
    }
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
