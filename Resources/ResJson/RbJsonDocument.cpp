#include "RbJsonDocument.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>

namespace usagi
{
ResourceState RbJsonDocument::construct(
    ResourceConstructDelegate<RbJsonDocument> &delegate)
{
    const auto res = delegate.resource<
        RbAssetMemoryView
    >(arg<AssetPath>()).await();
    const ReadonlyMemoryView src = res.value();
    assert(src);

    delegate.allocate(nlohmann::json::parse(src.to_string_view()));

    return ResourceState::READY;
}
}
