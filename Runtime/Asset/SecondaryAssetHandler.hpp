#pragma once

#include <future>
#include <memory>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "Asset.hpp"
#include "SecondaryAsset.hpp"

namespace usagi
{
class TaskExecutor;

// todo: allow multiple passes of dependency loading
class SecondaryAssetHandlerBase : Noncopyable
{
public:
    virtual ~SecondaryAssetHandlerBase() = default;

    // typeid doesn't work well with smart pointers. So instead of
    // `typeid(*smart_ptr)`, let the handler report its identity by itself.
    const std::type_info & type() const
    {
        return typeid(*this);
    }

protected:
    friend class AssetManager;
    friend class SecondaryAssetLoadingTask;

    virtual std::unique_ptr<SecondaryAsset> construct() = 0;

    struct Hasher
    {
        virtual ~Hasher() = default;

        virtual Hasher & append(const void *data, std::size_t size) = 0;

        Hasher & append(std::string_view str)
        {
            return append(str.data(), str.size());
        }
    };

    virtual void append_features(Hasher &hasher) = 0;

    [[nodiscard]]
    std::shared_future<PrimaryAssetMeta> primary_asset_async(
        std::string_view asset_path);

    [[nodiscard]]
    std::shared_future<SecondaryAssetMeta> secondary_asset_async(
        std::unique_ptr<SecondaryAssetHandlerBase> handler);

private:
    std::unique_ptr<SecondaryAsset> construct_with(
        AssetManager &asset_manager,
        TaskExecutor &work_queue);

    AssetManager *mManager = nullptr;
    TaskExecutor *mExecutor = nullptr;
};

template <typename SecondaryAssetType>
class SecondaryAssetHandler : public SecondaryAssetHandlerBase
{
public:
    using SecondaryAssetT = SecondaryAssetType;
};
}
