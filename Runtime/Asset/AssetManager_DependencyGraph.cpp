#include "AssetManager2.hpp"

#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
void AssetManager2::register_dependency(
    AssetHashId requester,
    AssetHashId target)
{
    LOG(trace, "[Asset] Adding edge Asset {:#0x} -> Asset {:#0x}",
        target, requester);
    std::unique_lock lk(mDependencyGraphMutex);
    mDependencies[target].emplace(requester);
    mDependencyReversed[requester].emplace(target);
}

void AssetManager2::register_dependency_nolock(
    AssetHashId requester,
    AssetPackage *target)
{
    LOG(trace, "[Asset] Adding edge Package {:#0x} -> Asset {:#0x}",
        (std::uint64_t)target, requester);
    mDependencies[target].emplace(requester);
    mDependencyReversed[requester].emplace(target);
}

bool AssetManager2::has_dependency_edge(
    const Vertex from,
    const AssetHashId to)
{
    std::shared_lock lk(mDependencyGraphMutex);
    const auto it = mDependencies.find(from);
    if(it == mDependencies.end()) return false;
    return it->second.contains(to);
}

void AssetManager2::erase_dependency_edge_nolock(Vertex from, AssetHashId to)
{
    const auto it = mDependencies.find(from);
    assert(it != mDependencies.end());
    // LOG(debug, "[Asset] Removing edge: {:#0x} -> {:#0x}", from, to);

    if(std::holds_alternative<AssetHashId>(from))
        LOG(debug, "[Asset] Removing dependency: {:#0x} -> {:#0x}",
            std::get<AssetHashId>(from), to);
    else
        LOG(debug, "[Asset] Removing dependency: {:#0x} -> {:#0x}",
            (std::uint64_t)std::get<AssetPackage *>(from), to);

    it->second.erase(to);
    // if(it->second.empty()) mDependencies.erase(it);
}
}
