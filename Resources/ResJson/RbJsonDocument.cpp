#include "RbJsonDocument.hpp"

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>

namespace usagi
{
ResourceState RbJsonDocument::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path)
{
    const auto res_raw = delegate.resource<RbAssetMemoryView>(path).await();
    const ReadonlyMemoryView src = *res_raw;
    assert(src);
    
    delegate.emplace(nlohmann::json::parse(src.to_string_view()));

    return ResourceState::READY;
}
}
