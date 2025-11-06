#pragma once

#include <Usagi/Library/Container/FixedCapacityString.hpp>

namespace usagi
{
struct ComponentScript
{
    FixedCapacityString<32> source_asset_path;
};
}
