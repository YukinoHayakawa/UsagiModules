#pragma once

#include <Usagi/Library/Container/FixedCapacityString.hpp>

namespace usagi
{
struct ComponentScriptPCH
{
    FixedCapacityString<32> src_asset_path;
    FixedCapacityString<32> bin_asset_path;
};
}
