#pragma once

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>

namespace usagi
{
class JsonTree : public SecondaryAsset
{
public:
    const nlohmann::json tree;

    explicit JsonTree(nlohmann::json tree)
        : tree(std::move(tree))
    {
    }
};
}
