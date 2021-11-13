#include "SahProgramModule.hpp"

#include <cassert>

#include <Usagi/Library/Utility/Variant.hpp>
#include <Usagi/Modules/Runtime/Asset/SecondaryAsset.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/ClangJIT.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/RuntimeModule.hpp>
#include <Usagi/Modules/Runtime/ProgramModule/CompilerInvocation.hpp>

namespace usagi
{
SahProgramModule::SahProgramModule(ClangJIT &jit)
    : mJit(jit)
{
}

SahProgramModule & SahProgramModule::set_pch(
    std::string asset_name_source,
    std::string asset_name_bin)
{
    mPchSource = std::move(asset_name_source);
    mPchBinary = std::move(asset_name_bin);

    return *this;
}

SahProgramModule & SahProgramModule::add_asset_source(std::string asset_name)
{
    mSources.emplace_back(AssetSource { .name = std::move(asset_name) });

    return *this;
}

SahProgramModule & SahProgramModule::add_string_source(
    std::string name,
    std::string text)
{
    mSources.emplace_back(StringSource {
        .name = std::move(name),
        .text = std::move(text)
    });

    return *this;
}

std::optional<std::string_view> SahProgramModule::primary_dependencies(
    std::size_t index)
{
    if(index < mSources.size())
    {
        if(auto &src = mSources[index]; src.index() == 1)
            return std::get<AssetSource>(src).name;
        return std::string_view { };
    }
    if(index == mSources.size()) return mPchSource;
    if(index == mSources.size() + 1) return mPchBinary;

    return { };
}

std::unique_ptr<SecondaryAsset> SahProgramModule::construct(
    const std::span<std::optional<PrimaryAssetMeta>> primary_assets)
{
    assert(primary_assets.size() == mSources.size() + 2);

    auto compiler = mJit.create_compiler();

    if(!mPchSource.empty())
    {
        assert(!mPchBinary.empty());
        const auto idx_src = mSources.size();
        const auto idx_bin = idx_src + 1;
        const auto reg_src = primary_assets[idx_src]->region;
        const auto reg_bin = primary_assets[idx_bin]->region;
        assert(reg_src && reg_bin);
        compiler.set_pch(reg_src, reg_bin, mPchBinary);
    }

    for(std::size_t idx = 0; auto &&s : mSources)
    {
        auto [name, source] = visit_element(s,
            [&](StringSource &ss) { return std::pair {
                std::string_view(ss.name),
                ReadonlyMemoryRegion::from_string_view(ss.text)
            }; },
            [&](AssetSource &as) { return std::pair {
                std::string_view(as.name),
                primary_assets[idx]->region
            }; }
        );
        compiler.add_source(name, source);
        ++idx;
    }

    auto mdl = compiler.compile();

    return std::make_unique<SecondaryAssetAdapter<RuntimeModule>>(
        std::move(mdl)
    );
}

void SahProgramModule::append_build_parameters(Hasher &hasher)
{
    hasher.append(mPchSource);
    hasher.append(mPchBinary);

    for(auto &&s : mSources)
    {
        visit_element(s,
            [&](StringSource &ss) { hasher.append(ss.name).append(ss.text); },
            [&](AssetSource &as) { hasher.append(as.name); }
        );
    }
}
}
