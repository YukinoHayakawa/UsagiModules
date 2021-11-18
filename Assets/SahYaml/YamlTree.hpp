#pragma once

#include <ryml/ryml.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>

namespace usagi
{
class YamlTree : public SecondaryAsset
{
public:
    const ryml::Tree tree;

    explicit YamlTree(ryml::Tree tree)
        : tree(std::move(tree))
    {
    }
};
}
