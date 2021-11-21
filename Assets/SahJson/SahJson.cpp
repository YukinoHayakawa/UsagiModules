#include "SahJson.hpp"

namespace usagi
{
std::unique_ptr<SecondaryAsset> SahJson::construct()
{
    const auto f = primary_asset_async(asset_path());
    const auto region = f.get().region;
    assert(region);

    nlohmann::json tree = nlohmann::json::parse(region.to_string_view());

    return std::make_unique<JsonTree>(std::move(tree));
}
}
