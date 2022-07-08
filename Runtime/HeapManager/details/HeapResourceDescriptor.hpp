#pragma once

#include <cstdint>

#include <fmt/format.h>

namespace usagi
{
using HeapResourceIdT = std::uint64_t;

class HeapResourceDescriptor
{
    // Check return type size of type_info.hash_code().
    static_assert(sizeof(std::size_t) <= sizeof(HeapResourceIdT));

    // todo maybe just merge these two into one? - no. because builder type sometime has to be verified against
    // typeid(HeapT).hash_code()
    HeapResourceIdT mBuilderTypeHash { };
    // (..., ResourceHasher.append(build_args));
    HeapResourceIdT mBuildParamHash { };

public:
    HeapResourceDescriptor() = default;

    HeapResourceDescriptor(
        HeapResourceIdT builder_type_hash,
        HeapResourceIdT build_param_hash)
        : mBuilderTypeHash(builder_type_hash)
        , mBuildParamHash(build_param_hash)
    {
    }

    HeapResourceIdT builder_id() const { return mBuilderTypeHash; }
    HeapResourceIdT resource_id() const { return mBuildParamHash; }

    explicit operator bool() const
    {
        return mBuilderTypeHash && mBuildParamHash;
    }

    auto operator<=>(const HeapResourceDescriptor &rhs) const = default;
};

inline std::string_view to_string_view(const HeapResourceDescriptor descriptor)
{
    return {
        reinterpret_cast<const char *>(&descriptor),
        sizeof(HeapResourceDescriptor)
    };
}
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
            d.builder_id(),
            d.resource_id()
        );
    }
};
