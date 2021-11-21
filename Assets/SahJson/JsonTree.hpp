#pragma once

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>

namespace usagi
{
using JsonTree = SecondaryAssetAdapter<nlohmann::json>;
}
