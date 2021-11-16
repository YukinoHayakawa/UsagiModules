#include "SahProgramModule.hpp"

#include <cassert>

#include <Usagi/Library/Utility/Variant.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetManager.hpp>
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
    std::string asset_name_bin,
    std::string source_remapped_name)
{
    mAssetPathPchSource = std::move(asset_name_source);
    mAssetPathPchBinary = std::move(asset_name_bin);
    mPchSourceRemappedName = std::move(source_remapped_name);

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

std::unique_ptr<SecondaryAsset> SahProgramModule::construct(
    AssetManager &asset_manager,
    TaskExecutor &work_queue)
{
    // Prepare assets

    std::vector<std::shared_future<PrimaryAssetMeta>> assets;
    assets.resize(mSources.size() + 2);

    auto load = [&](std::uint64_t idx, std::string_view path) {
        assets[idx] = asset_manager.primary_asset_async(path, work_queue);
    };

    for(std::size_t idx = 0; auto &&s : mSources)
    {
        visit_element(s,
            [&](const StringSource &ss) { },
            [&](const AssetSource &as) { load(idx, as.name); }
        );
        ++idx;
    }

    if(!mAssetPathPchSource.empty())
    {
        const auto idx_src = mSources.size();
        const auto idx_bin = idx_src + 1;
        assert(!mAssetPathPchBinary.empty());
        load(idx_src, mAssetPathPchSource);
        load(idx_bin, mAssetPathPchBinary);
    }

    for(auto &&f : assets)
        if(f.valid()) f.wait();

    // Compile the source

    auto get = [&](std::uint64_t idx) { return assets[idx].get().region; };

    auto compiler = mJit.create_compiler();

    if(!mAssetPathPchSource.empty())
    {
        const auto idx_src = mSources.size();
        const auto idx_bin = idx_src + 1;
        const auto reg_src = get(idx_src);
        const auto reg_bin = get(idx_bin);
        assert(reg_src && reg_bin);
        compiler.set_pch(reg_src, reg_bin, mPchSourceRemappedName);
    }

    for(std::size_t idx = 0; auto &&s : mSources)
    {
        auto [name, source] = visit_element(s,
            [&](const StringSource &ss) { return std::pair {
                std::string_view(ss.name),
                ReadonlyMemoryRegion::from_string_view(ss.text)
            }; },
            [&](const AssetSource &as) { return std::pair {
                std::string_view(as.name),
                get(idx)
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

void SahProgramModule::append_features(Hasher &h)
{
    h.append(mAssetPathPchSource).append(mAssetPathPchBinary);

    for(auto &&s : mSources)
    {
        visit_element(s,
            [&](const StringSource &ss) { h.append(ss.name).append(ss.text); },
            [&](const AssetSource &as) { h.append(as.name); }
        );
    }
}
}
