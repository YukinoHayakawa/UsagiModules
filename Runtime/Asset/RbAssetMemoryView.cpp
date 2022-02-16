#include "RbAssetMemoryView.hpp"

#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
ResourceState RbAssetMemoryView::construct(
    ResourceConstructDelegate<RbAssetMemoryView> &delegate)
{
    /*
     * Locate the asset package containing the requested asset following
     * overriding rule. See AssetPackageManager for details.
     */
    MemoryArena arena;
    // The query goes through asset manager which sync the accesses.
    auto [status, query] = delegate.allocate(
        arg<AssetPath>(),
        arena
    );

    if(status == AssetStatus::MISSING)
        return ResourceState::FAILED_INVALID_DATA;
    if(status == AssetStatus::EXIST_BUSY)
        return ResourceState::FAILED_BUSY;
    if(status == AssetStatus::EXIST)
    {
        query->fetch();
        assert(query->ready());
		return ResourceState::READY;
    }

    USAGI_UNREACHABLE("Invalid asset status.");
}
}
