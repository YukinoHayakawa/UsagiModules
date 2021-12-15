#include "AssetAccessProxy.hpp"

#include <cassert>

#include "AssetRecord.hpp"

namespace usagi
{
void AssetAccessProxy::grab_state_snapshot()
{
    // Make sure that if status indicates the asset is ready, the pointer
    // must be valid.
    mStateSnapshot.status = mRecord->status.load(std::memory_order::acquire);
    mStateSnapshot.asset = mRecord->asset.get();
    assert((mStateSnapshot.status != AssetStatus::READY
        || mStateSnapshot.asset)
        && "If the asset is ready, the pointer must be set.");
}

AssetAccessProxy::AssetAccessProxy(const AssetHashId id, AssetRecord *record)
    : mId(id)
    , mRecord(record)
{
    if(has_asset()) grab_state_snapshot();
    else mStateSnapshot.status = AssetStatus::MISSING;
}

Asset * AssetAccessProxy::maybe_asset() const
{
    if(has_asset())
    {
        assert(status() == AssetStatus::READY);
        return mStateSnapshot.asset;
    }
    return { };
}

Asset * AssetAccessProxy::await_asset()
{
    if(!has_asset()) return { };

    // todo: is it safer to perform a new request via the AssetManger? grabbing the state by ourself principally won't cause problems because the invariant is that if the future object is ready, it must be the case either the asset is ready, or the asset was failed to load.
    mRecord->future.wait();
    grab_state_snapshot();

    assert((status() == AssetStatus::READY
        || status() == AssetStatus::FAILED)
        && "If the future object is ready, the asset must be either "
        "ready or failed to load.");

    return maybe_asset();
}
}
