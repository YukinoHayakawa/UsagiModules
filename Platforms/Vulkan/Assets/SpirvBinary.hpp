#pragma once

#include <vector>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>

namespace usagi
{
using SpirvBinary = SecondaryAssetAdapter<std::vector<std::uint32_t>>;
}
