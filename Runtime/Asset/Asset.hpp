#pragma once

#include <cassert>

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Runtime/Memory/View.hpp>

namespace usagi
{
/*
 * Base class for assets.
 */
class Asset : Noncopyable
{
public:
    virtual ~Asset() = default;

    template <typename T>
    T & as()
    {
        assert(dynamic_cast<T *>(this));
        return static_cast<T &>(*this);
    }
};

class AssetRawMemoryView final : public Asset
{
    ReadonlyMemoryView mMemory;

public:
    explicit AssetRawMemoryView(ReadonlyMemoryView memory)
        : mMemory(memory)
    {
    }

    ReadonlyMemoryView memory() const
    {
        return mMemory;
    }
};
//
//
// // Asset fingerprint is used for validate the content of assets. For primary
// // assets, it is the hash of the asset content. For secondary assets, it
// // reflects the content of primary dependencies and secondary asset build
// // parameters.
// using AssetFingerprint = std::uint64_t;
//
//
// struct SecondaryAssetMeta
// {
//     class SecondaryAsset *asset = nullptr;
//     // Secondary asset fingerprint consists of two parts. The build fingerprint
//     // is the hash of secondary asset handler type and build parameters, which
//     // is used for querying in the cache for the secondary asset. The
//     // dependency fingerprint is the hash of fingerprints of primary assets
//     // used when building the secondary asset. It can be used to detect outdated
//     // secondary cache entries when the content of dependent assets changes.
//     AssetFingerprint fingerprint_build = 0;
//     // todo: hash the asset product?
//     AssetFingerprint fingerprint_dep_content = 0;
//     // AssetStatus status = AssetStatus::MISSING;
// };
}
