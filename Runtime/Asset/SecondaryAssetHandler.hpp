#pragma once

#include <memory>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "SecondaryAsset.hpp"

namespace usagi
{
class TaskExecutor;

// todo: allow multiple passes of dependency loading
class SecondaryAssetHandlerBase : Noncopyable
{
public:
    virtual ~SecondaryAssetHandlerBase() = default;

protected:
    friend class AssetManager;
    friend class SecondaryAssetLoadingTask;

    virtual std::unique_ptr<SecondaryAsset> construct(
        AssetManager &asset_manager,
        TaskExecutor &work_queue) = 0;

    struct Hasher
    {
        virtual ~Hasher() = default;

        virtual Hasher & append(const void *data, std::size_t size) = 0;

        Hasher & append(std::string_view str)
        {
            return append(str.data(), str.size());
        }
    };

    virtual void append_features(Hasher &hasher) {}
};

template <typename SecondaryAssetType>
class SecondaryAssetHandler : public SecondaryAssetHandlerBase
{
public:
    using SecondaryAssetT = SecondaryAssetType;
};
}
