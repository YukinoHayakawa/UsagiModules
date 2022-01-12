#include "AbRawMemoryView.hpp"

#include <Usagi/Library/Memory/MemoryArena.hpp>
#include <Usagi/Modules/Runtime/Asset/AssetManager2.hpp>

#include "AssetQuery.hpp"
#include "AssetRequestProxy.hpp"

namespace usagi
{
AbRawMemoryView::AbRawMemoryView(const AssetPath path)
    : mCleanAssetPath(path.reconstructed())
{
}

ReturnValue<AssetStatus, std::unique_ptr<AssetRawMemoryView>>
AbRawMemoryView::construct_with(AssetRequestProxy request_proxy) const
{
    /*
     * Locate the asset package containing the requested asset following
     * overriding rule. See AssetPackageManager for details.
     */
    MemoryArena arena;
    // The query goes through asset manager to have proper synchronization.
    auto [status, query] = request_proxy.create_asset_query(
        { mCleanAssetPath },
        arena
    );

    switch(status)
    {
        case AssetStatus::MISSING:
        case AssetStatus::EXIST_BUSY:
            return { status, { } };
        case AssetStatus::EXIST:
            break;
        default: USAGI_INVALID_ENUM_VALUE();
    }

    /*
     * Preload the bytes into memory so that subsequent reads will unlikely
     * cause page faults.
     * todo: partial load?
     * todo: hash content?
     */
    query->fetch();
    assert(query->ready());

    return {
        AssetStatus::READY,
        std::make_unique<AssetRawMemoryView>(
            query->package(),
            query->data()
        )
    };
}
}
