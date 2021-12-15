﻿#pragma once

#include "AssetDefs.hpp"
#include "AssetEnum.hpp"
#include "AssetState.hpp"

namespace usagi
{
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
class AssetAccessProxy
{
    AssetHashId mId = 0;
    AssetRecord *mRecord;
    AssetState mStateSnapshot;

    // Shares the ownership of the asset with AssetManager to prevent it
    // from being evicted while in use.
    AssetReference mRefCount;

    bool has_asset() const { return mRecord; }
    void grab_state_snapshot();

public:
    AssetAccessProxy(AssetHashId id, AssetRecord *record);

    // Can be used for faster subsequent requests.
    AssetHashId id() const { return mId; }

    /*
     * The status of the asset at the time of request. It may be changed after
     * this proxy object is constructed, except that it will not be evicted
     * if it is already loaded or loading, because this object holds a reference
     * to the asset.
     */
    AssetStatus status() const { return mStateSnapshot.status; }

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
};
}
