#include "SahProgramModule.hpp"

#include <cassert>

#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/ClangJIT.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/RuntimeModule.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/CompilerInvocation.hpp>

namespace usagi
{
SahProgramModule::SahProgramModule(
    ClangJIT &jit,
    std::string pch,
    std::string src)
    : mJit(jit)
    , mPch(std::move(pch))
    , mSource(std::move(src))
{
}

std::optional<std::string_view> SahProgramModule::primary_dependencies(
    std::size_t index)
{
    if(index == 0) return mPch;
    if(index == 1) return mSource;
    return { };
}

std::unique_ptr<SecondaryAsset> SahProgramModule::construct(
    const std::span<std::optional<PrimaryAssetMeta>> primary_assets)
{
    assert(primary_assets.size() == 2);

    auto compiler = mJit.create_compiler();
    if(!mPch.empty())
    {
        assert(primary_assets[0]->region);
        compiler.set_pch(primary_assets[0]->region);
    }
    compiler.add_source("<source>", primary_assets[1]->region);

    auto mdl = compiler.compile();

    return std::make_unique<SecondaryAssetAdapter<RuntimeModule>>(
        std::move(mdl)
    );
}

void SahProgramModule::append_build_parameters(Hasher &hasher)
{
    // todo
}
}
