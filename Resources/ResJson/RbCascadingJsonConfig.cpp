#include "RbCascadingJsonConfig.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>

#include "RbJsonDocument.hpp"

namespace usagi
{
ResourceState RbCascadingJsonConfig::construct(
    ResourceConstructDelegate<RbCascadingJsonConfig> &delegate)
{
    // Fetch the content of the requested config file
    const auto doc = delegate.resource<RbJsonDocument>(
        arg<AssetPath>()
    ).await();
    const auto &cur = doc->root;

    // Extract the base config asset path
    const auto it = cur.find("inherit");

    // The config doesn't overrides another. We are done here.
    if(it == cur.end())
    {
        delegate.allocate(cur);
        return ResourceState::READY;
    }

    // Otherwise, trace to the parent config it inherits.
    assert(it->is_string());
    std::string base_path = *it;

    // Recursively request the parent config tree and make a copy of it.
    auto parent_doc = delegate.resource<RbCascadingJsonConfig>(
        base_path
    ).await()->root;

    parent_doc.merge_patch(cur);
    parent_doc.erase("inherit");

    delegate.allocate(std::move(parent_doc));

    return ResourceState::READY;
}
}
