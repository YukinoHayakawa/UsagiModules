#pragma once

#include "AssetDefs.hpp"
#include "AssetEnum.hpp"

namespace usagi
{
class Asset;
struct AssetRecord;
class AssetManager2;

/*
 * User: Ask for an asset.
 * Is asset ready?
 * Y: Returns the asset.
 * N: Reason?
 * N: User: Wait.
 * N: User: Ignore.
 * Wait: Returns latest state?
 */
class AssetAccessProxyBase
{
    AssetHashId mId = 0;
    AssetRecord *mRecord;
    // Snapshot of asset ptr & status when this proxy is constructed.
    Asset *mAsset = nullptr;
    AssetStatus mStatus = AssetStatus::MISSING;

    // Shares the ownership of the asset with AssetManager to prevent it
    // from being evicted while in use.
    AssetReference mRefCount;

protected:
    bool has_record() const { return mRecord; }
    void grab_state_snapshot();

    AssetAccessProxyBase(AssetHashId id, AssetRecord *record);

    /*
     * Get the reference to the asset if it is ready.
     */
    Asset * maybe_asset() const;

    /*
     * Wait for the latest state of the asset. Note that this will update
     * the asset status and affect the results of subsequent queries to
     * the asset and its status via this proxy object.
     */
    Asset * await_asset();

public:
    // Can be used for faster subsequent requests.
    AssetHashId id() const { return mId; }

    /*
    * The status of the asset at the time of request. It may be changed after
    * this proxy object is constructed, except that it will not be evicted
    * if it is already loaded or loading, because this object holds a reference
    * to the asset.
    */
    AssetStatus status() const { return mStatus; }
};

template <typename AssetT>
class AssetAccessProxy : public AssetAccessProxyBase
{
protected:
    // Only allows asset manager to construct this accessor
    friend class AssetManager2;
    using AssetAccessProxyBase::AssetAccessProxyBase;

public:
    AssetT * maybe_asset() const
    {
        return static_cast<AssetT *>(AssetAccessProxyBase::maybe_asset());
    }

    AssetT * await_asset()
    {
        return static_cast<AssetT *>(AssetAccessProxyBase::await_asset());
    }
};
}
