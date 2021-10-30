#pragma once

#include <string>

#include <Usagi/Modules/Runtime/Asset/SecondaryAssetHandler.hpp>

namespace usagi
{
class RuntimeModule;
class ClangJIT;

class SahProgramModule
    : public SecondaryAssetHandler<RuntimeModule>
{
    ClangJIT &mJit;
    std::string mPch, mSource;

public:
    SahProgramModule(ClangJIT &jit, std::string pch, std::string src);

    [[nodiscard]]
    std::optional<std::string_view> primary_dependencies(
        std::size_t index) override;

    [[nodiscard]]
    std::unique_ptr<SecondaryAsset> construct(
        std::span<std::optional<PrimaryAssetMeta>> primary_assets) override;

    void append_build_parameters(Hasher &hasher) override;
};
}
