#pragma once

#include <Usagi/Modules/Services/Asset/Asset.hpp>
#include <Usagi/Runtime/Memory/Region.hpp>

namespace usagi
{
class AssetSource;
class AssetManager;

class AssetBuilderRawBuffer
{
    AssetManager *mManager = nullptr;
    std::u8string mAssetLocator;
    AssetSource *mPackage = nullptr;

public:
    AssetBuilderRawBuffer(AssetManager *manager, std::u8string asset_locator);

    using OutputT = MemoryRegion;

    AssetHandle hash() const;
    MemoryRegion build();
    static void free(const MemoryRegion &mem);
};
static_assert(AssetBuilder<AssetBuilderRawBuffer>);
}
