#pragma once

#include "../External/xxhash/xxhash64.h"

#include <Usagi/Library/Utilities/ByteView.hpp>

#include <Usagi/Modules/Runtime/HeapManager/TransparentArg.hpp>

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
        auto mem_view = to_byte_view(val);
		[[maybe_unused]]
        const bool result = mHasher.add(mem_view.data(), mem_view.size());
		assert(result);
        mNumProcessedBytes += mem_view.size();
        return mNumProcessedBytes - current;
	}

	// ignore transparent arguments
	template <typename T>
    std::size_t append(TransparentArg<T>)
	{
	    return mNumProcessedBytes;
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
}
