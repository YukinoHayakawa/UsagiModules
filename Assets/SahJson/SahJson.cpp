﻿#include "SahJson.hpp"

namespace usagi
{
std::unique_ptr<SecondaryAsset> SahJson::construct()
{
    const auto f = primary_asset_async(mAssetPath);
    const auto region = f.get().region;
    assert(region);

    nlohmann::json tree = nlohmann::json::parse(region.to_string_view());

    return std::make_unique<JsonTree>(std::move(tree));
}

void SahJson::append_features(Hasher &hasher)
{
    hasher.append(mAssetPath);
}
}
