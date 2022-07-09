#pragma once

#include <nlohmann/json_fwd.hpp>

#include <Usagi/Modules/Runtime/Asset/AssetPath.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

namespace usagi
{
class RbJsonDocument : public ResourceBuilderDecl<
    nlohmann::json,
    const AssetPath &>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        const AssetPath &path) override;
};
}
