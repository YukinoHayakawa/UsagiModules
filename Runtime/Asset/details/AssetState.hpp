#pragma once

#include "AssetEnum.hpp"

namespace usagi
{
class Asset;

struct AssetState
{
    Asset *asset = nullptr;
    AssetStatus status = AssetStatus::MISSING;
};
}
