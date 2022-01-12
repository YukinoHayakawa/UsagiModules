#pragma once

#include <Usagi/Runtime/Memory/View.hpp>

#include "../Asset.hpp"

namespace usagi
{
class AssetPackage;

class AssetRawMemoryView final : public Asset
{
    AssetPackage *mPackage;
    ReadonlyMemoryView mMemory;

public:
    AssetRawMemoryView(AssetPackage *package, ReadonlyMemoryView memory)
        : mPackage(package)
        , mMemory(std::move(memory))
    {
    }

    AssetPackage * package() const
    {
        return mPackage;
    }

    ReadonlyMemoryView memory() const
    {
        return mMemory;
    }
};
}
