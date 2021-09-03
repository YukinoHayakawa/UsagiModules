#include "AssetBuilderRawBuffer.hpp"

#include <Usagi/Module/Service/Asset/AssetManager.hpp>
#include <Usagi/Module/Service/Asset/Crc32.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
AssetBuilderRawBuffer::AssetBuilderRawBuffer(
    AssetManager *manager,
    std::u8string asset_locator)
    : mManager(manager)
    , mAssetLocator(std::move(asset_locator))
{
    // todo find from specific package
    mPackage = mManager->try_find_source(mAssetLocator);

    // if asset was not found, throw an exception.
    if(mPackage == nullptr)
        USAGI_THROW(AssetNotFound(mAssetLocator));
}

AssetHandle AssetBuilderRawBuffer::hash() const
{
    AssetHandle crc = 0;

    crc = crc32c(crc, mPackage);
    // todo: use canonical path
    crc = crc32c(crc, mAssetLocator.data(), mAssetLocator.size());

    return crc;
}

MemoryRegion AssetBuilderRawBuffer::build()
{
    const auto blob = mPackage->load(mAssetLocator);
    return blob;
}

void AssetBuilderRawBuffer::free(const MemoryRegion &mem)
{
    // todo impl
    throw std::runtime_error("unimplemented");
}
}
