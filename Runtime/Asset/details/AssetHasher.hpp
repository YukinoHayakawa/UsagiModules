#pragma once

#include <string_view>

#include "../External/xxhash/xxhash64.h"

namespace usagi
{
class AssetHasher
{
	XXHash64 mHasher { 0 };
	std::size_t mNumProcessedBytes = 0;

public:
	template <typename T>
    std::size_t append(T &&val)
	{
		const auto current = mNumProcessedBytes;
		// Use ADL to find the append function
		append_bytes([&](std::string_view mem_view) {
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

void append_bytes(auto append_func, auto val)
    requires std::is_arithmetic_v<decltype(val)>
{
    std::string_view view { reinterpret_cast<const char *>(&val), sizeof(val) };
    append_func(view);
}

void append_bytes(auto append_func, std::string_view val)
{
    append_func(val);
}

void append_bytes(auto append_func, std::string val)
{
    append_func(val);
}

void append_bytes(auto append_func, const char *val)
{
    append_bytes(std::move(append_func), std::string_view(val));
}
}
