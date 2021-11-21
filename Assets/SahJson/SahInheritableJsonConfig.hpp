#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "JsonTree.hpp"

namespace usagi
{
class SahInheritableJsonConfig
    : public SingleDependencySecondaryAssetHandler<JsonTree>
{
public:
    using BaseT::BaseT;

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
};
}
