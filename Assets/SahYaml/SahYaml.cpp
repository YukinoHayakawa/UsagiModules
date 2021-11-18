#include "SahYaml.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetManager.hpp>

namespace usagi
{
void SahYaml::error(const char *msg, size_t len, ryml::Location loc, void *)
{
    LOG(error, "[YAML] Parsing error at {}:{} (bufpos={}): {}",
        loc.line, loc.col, loc.offset, msg);
}

SahYaml::SahYaml(std::string asset_path): mAssetPath(std::move(asset_path))
{
    ryml::set_callbacks({ nullptr, nullptr, nullptr, &error });
}

std::unique_ptr<SecondaryAsset> SahYaml::construct()
{
    const auto f = primary_asset_async(mAssetPath);
    auto &src = f.get();
    assert(src.region);

    const auto view = src.region.bom_free_string_view();
    ryml::Tree tree = ryml::parse(ryml::csubstr(view.data(), view.size()));

    return std::make_unique<YamlTree>(std::move(tree));
}

void SahYaml::append_features(Hasher &hasher)
{
    hasher.append(mAssetPath);
}
}
