#include "RbCascadingJsonConfig.hpp"

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Runtime/Asset/RbAssetMemoryView.hpp>

#include "RbJsonDocument.hpp"

namespace usagi
{
ResourceState RbCascadingJsonConfig::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &asset_path) 
{
    constexpr auto KEY_INHERIT = "inherit";

    // Fetch the raw content of the requested config file
    const auto doc = delegate.resource<RbJsonDocument>(asset_path).await();
    const auto &cur = *doc;

    // Get the key that contains documents to be included.
    const auto it = cur.find(KEY_INHERIT);

    // The config doesn't overrides another. We are done here. Return as-is.
    if(it == cur.end())
    {
        // copy it
        delegate.emplace(cur);
        return ResourceState::READY;
    }

    // When there is inheritance, the inherit key is checked. If it is an array
    // of strings, the first document specified in the array will be loaded
    // and used as the base document. The subsequent documents in the array
    // will be loaded and applied as patch in the declared order. The content
    // of the current document will be applied as patch as last and the inherit
    // key will be removed when the process is finished. If a string is
    // specified for the inherit key, the process proceeds as if there is only
    // one string in the array.

    auto &out = delegate.emplace();

    const auto merge_parent = [&](const std::string &parent_path)
    {
        // Request parent document. The described process must have already
        // been done to it. Request as transient resource since the documents
        // must be applied sequentially.
        const auto &parent_doc = 
            delegate.resource_transient<RbCascadingJsonConfig>(parent_path)
                .await().cref();

        USAGI_ASSERT_THROW(
            !parent_doc.contains(KEY_INHERIT), 
            std::runtime_error(
                "The inherit key of parent document should have been removed."
            )
        );

        out.merge_patch(parent_doc);
    };

    if(it->is_array())
    {
        for(auto &[key, val] : it->items())
        {
            USAGI_ASSERT_THROW(
                val.is_string(),
                std::runtime_error(
                    "Path to parent document should be a string."
                )
            );
            merge_parent(val.get_ref<const std::string &>());
        }
    }
    else if(it->is_string())
    {
        merge_parent(it->get_ref<const std::string &>());
    }
    else
    {
        USAGI_THROW(
            std::runtime_error(
                "inherit key should be either a string or an array of strings."
            )
        );
    }

    // Apply initial document.
    out.merge_patch(cur);
    // Remove the inherit key from the result document.
    out.erase("inherit");

    return ResourceState::READY;
}
}
