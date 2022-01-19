#pragma once

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Runtime/HeapManager/Resource.hpp>

namespace usagi
{
struct JsonDocument : Resource
{
    nlohmann::json root;

    explicit JsonDocument(nlohmann::json root)
        : root(std::move(root))
    {
    }
};
}
