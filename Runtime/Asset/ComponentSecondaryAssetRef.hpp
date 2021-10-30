#pragma once

#include "Asset.hpp"

namespace usagi
{
struct ComponentSecondaryAssetRef
{
    AssetFingerprint fingerprint_build = 0;
    AssetFingerprint fingerprint_dep_content = 0;
};
}
