#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "JsonTree.hpp"

namespace usagi
{
class SahInheritableJsonConfig : public SecondaryAssetHandler<JsonTree>
{
    std::string mRequestedAssetPath;

public:
    explicit SahInheritableJsonConfig(std::string start_asset_path);

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
    void append_features(Hasher &hasher) override;
};
}
