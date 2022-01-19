#pragma once

#include <Usagi/Concept/Type/Types.hpp>

#include "../External/xxhash/xxhash64.h"

namespace usagi
{
template <typename T>
struct AppendToHasher;

class ResourceHasher
{
	XXHash64 mHasher { 0 };
	std::size_t mNumProcessedBytes = 0;

public:
	template <typename T>
    std::size_t append(T &&val)
	{
		const auto current = mNumProcessedBytes;
        AppendToHasher<T> appender;
		appender([&](std::string_view mem_view) {
			mHasher.add(mem_view.data(), mem_view.size());
			mNumProcessedBytes += mem_view.size();
		}, val);
        return mNumProcessedBytes - current;
	}

    std::size_t num_processed_bytes() const
	{
        return mNumProcessedBytes;
	}

	std::uint64_t hash() const
	{
        return mHasher.hash();
	}
};

template <Arithmetic T>
struct AppendToHasher<T>
{
    void operator()(auto &&append_func, const T &val)
    {
        std::string_view view {
            reinterpret_cast<const char *>(&val),
            sizeof(T)
        };
        append_func(view);
    }
};

template <StringView T>
struct AppendToHasher<T>
{
    void operator()(auto &&append_func, const T &val)
    {
        append_func(std::string_view(val));
    }
};

template <Enum T>
struct AppendToHasher<T>
{
    template <typename Func>
    void operator()(Func &&append_func, const T &val)
    {
        using UnderlyingT = std::underlying_type_t<std::remove_cvref_t<T>>;
        AppendToHasher<UnderlyingT> appender;
        appender(
            std::forward<Func>(append_func),
            static_cast<UnderlyingT>(val)
        );
    }
};
}
