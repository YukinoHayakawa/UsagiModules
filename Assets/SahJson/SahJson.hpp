#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "JsonTree.hpp"

namespace usagi
{
class SahJson : public SecondaryAssetHandler<JsonTree>
{
    std::string mAssetPath;

public:
    explicit SahJson(std::string asset_path)
        : mAssetPath(std::move(asset_path))
    {
    }

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
