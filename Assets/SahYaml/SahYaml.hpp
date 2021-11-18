#pragma once

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

#include "YamlTree.hpp"

namespace usagi
{
class SahYaml : public SecondaryAssetHandler<YamlTree>
{
    static void error(const char* msg, size_t len, ryml::Location loc, void *);

    std::string mAssetPath;

public:
    explicit SahYaml(std::string asset_path);

protected:
    std::unique_ptr<SecondaryAsset> construct(
        AssetManager &asset_manager,
        TaskExecutor &work_queue) override;

    void append_features(Hasher &hasher) override;
};
}
