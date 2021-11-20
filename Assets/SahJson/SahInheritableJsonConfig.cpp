#include "SahInheritableJsonConfig.hpp"

#include "SahJson.hpp"

namespace usagi
{
SahInheritableJsonConfig::SahInheritableJsonConfig(std::string start_asset_path)
    : mRequestedAssetPath(std::move(start_asset_path))
{
}

std::unique_ptr<SecondaryAsset> SahInheritableJsonConfig::construct()
{
	// Fetch the content of the requested config file
    const auto f = secondary_asset_async(
        std::make_unique<SahJson>(mRequestedAssetPath)
	);
	// No need to copy the tree here because merging doesn't need to copy this
	// tree, it copies the parent tree instead.
	const auto &tree_ref = f.get().asset->as<JsonTree>()->tree;

	// Extract the base config asset path
    const auto it = tree_ref.find("inherit");
	// The config doesn't overrides another. We are done here.
	if(it == tree_ref.end())
	    return std::make_unique<JsonTree>(tree_ref);

    assert(it->is_string());
    std::string base_path = *it;

	// Recursively request the parent config tree
	const auto parent_f = secondary_asset_async(
        std::make_unique<SahInheritableJsonConfig>(base_path)
	);
	auto parent_tree = parent_f.get().asset->as<JsonTree>()->tree;
	parent_tree.merge_patch(tree_ref);
	parent_tree.erase("inherit");

    return std::make_unique<JsonTree>(std::move(parent_tree));
}

void SahInheritableJsonConfig::append_features(Hasher &hasher)
{
	hasher.append(mRequestedAssetPath);
}
}
