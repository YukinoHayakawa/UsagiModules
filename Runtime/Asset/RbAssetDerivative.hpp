#pragma once

#include <string>

namespace usagi
{
class RbAssetDerivative
{
    std::string mNormalizedAssetPath;

public:
    explicit RbAssetDerivative(std::string normalized_asset_path)
        : mNormalizedAssetPath(std::move(normalized_asset_path))
    {
    }

    const std::string & asset_path() const
    {
        return mNormalizedAssetPath;
    }
};
}
