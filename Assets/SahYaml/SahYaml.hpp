#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "YamlTree.hpp"

namespace usagi
{
class SahYaml : public SingleDependencySecondaryAssetHandler<YamlTree>
{
    static void error(const char *msg, size_t len, ryml::Location loc, void *);

public:
    using BaseT::BaseT;

protected:
    std::unique_ptr<SecondaryAsset> construct() override;
};
}
