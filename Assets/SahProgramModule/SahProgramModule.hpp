#pragma once

#include <vector>
#include <string>
#include <variant>

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

namespace usagi
{
class RuntimeModule;
class ClangJIT;

class SahProgramModule
    : public SecondaryAssetHandler<RuntimeModule>
{
    ClangJIT &mJit;

    std::string mAssetPathPchSource;
    std::string mAssetPathPchBinary;
    std::string mPchSourceRemappedName;

    struct StringSource
    {
        std::string name, text;
    };

    struct AssetSource
    {
        std::string name;
    };

    std::vector<std::variant<StringSource, AssetSource>> mSources;

    std::unique_ptr<SecondaryAsset> construct() override;

    void append_features(Hasher &h) override;

public:
    explicit SahProgramModule(ClangJIT &jit);

    SahProgramModule & set_pch(
        std::string asset_name_source,
        std::string asset_name_bin,
        std::string source_remapped_name);

    SahProgramModule & add_asset_source(
        std::string asset_name);

    SahProgramModule & add_string_source(
        std::string name,
        std::string text);
};
}
