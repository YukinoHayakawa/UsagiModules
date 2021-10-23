#pragma once

#include <memory>

#include "Asset.hpp"

namespace usagi
{
class SecondaryAssetHandler
{
public:
    virtual ~SecondaryAssetHandler() = default;

    // todo feed hasher
    virtual AssetCacheSignature signature() = 0;
    virtual std::unique_ptr<SecondaryAsset> construct(
        ReadonlyMemoryRegion primary) = 0;
};
}
