#pragma once

#include <Usagi/Runtime/Memory/View.hpp>

#include "../Asset.hpp"

namespace usagi
{
class AssetRawMemoryView final : public Asset
{
    ReadonlyMemoryView mMemory;

public:
    explicit AssetRawMemoryView(ReadonlyMemoryView memory)
        : mMemory(memory)
    {
    }

    ReadonlyMemoryView memory() const
    {
        return mMemory;
    }
};
}
