#include "RbJsonDocument.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>

namespace usagi
{
RbJsonDocument::RbJsonDocument(std::string normalized_asset_path)
    : RbAssetDerivative(std::move(normalized_asset_path))
{
}

ResourceState RbJsonDocument::construct(
    ResourceConstructDelegate<RbJsonDocument> &delegate)
{
    const ReadonlyMemoryView src = delegate.resource<
        RbAssetMemoryView
    >(asset_path()).await();
    assert(src);

    delegate.allocate(nlohmann::json::parse(src.to_string_view()));

    return ResourceState::READY;
}
}
