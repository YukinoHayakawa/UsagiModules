#pragma once

#include <future>
#include <memory>

#include <Usagi/Library/Memory/Noncopyable.hpp>

#include "Asset.hpp"
#include "SecondaryAsset.hpp"

namespace usagi
{
class TaskExecutor;

// class AssetProvider
// {
//     class AssetManager *mManager = nullptr;
//     class TaskExecutor *mExecutor = nullptr;
//
// public:
//     AssetProvider(AssetManager *manager, TaskExecutor *executor)
//         : mManager(manager)
//         , mExecutor(executor)
//     {
//     }
// };

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

    template <typename SecondaryAssetHandlerT, typename... Args>
    [[nodiscard]]
    std::shared_future<SecondaryAssetMeta> secondary_asset_async(Args &&...args)
    {
        return secondary_asset_async(std::make_unique<SecondaryAssetHandlerT>(
            std::forward<Args>(args)...
        ));
    }

    static auto & await(const std::shared_future<PrimaryAssetMeta> &future)
    {
        return future.get().region;
    }

    template <typename SecondaryAssetT>
    static auto & await(const std::shared_future<SecondaryAssetMeta> &future)
    {
        return future.get().asset->as<SecondaryAssetT>().value();
    }

    template <
        typename SecondaryAssetHandlerT,
        typename... Args
    >
    [[nodiscard]]
    auto & await_secondary(Args &&...args)
    {
        auto f = secondary_asset_async<SecondaryAssetHandlerT>(
            std::forward<Args>(args)...
        );
        return await<typename SecondaryAssetHandlerT::SecondaryAssetT>(f);
    }

private:
    std::unique_ptr<SecondaryAsset> construct_with(
        AssetManager &asset_manager,
        TaskExecutor &work_queue);

    class AssetManager *mManager = nullptr;
    class TaskExecutor *mExecutor = nullptr;
};

template <typename SecondaryAssetType>
class SecondaryAssetHandler : public SecondaryAssetHandlerBase
{
public:
    using SecondaryAssetT = SecondaryAssetType;
};

template <typename SecondaryAssetType>
class SingleDependencySecondaryAssetHandler
    : public SecondaryAssetHandler<SecondaryAssetType>
{
    const std::string mAssetPath;

protected:
    const std::string & asset_path() const
    {
        return mAssetPath;
    }

    template <typename SecondaryAssetHandlerT>
    [[nodiscard]]
    auto & await_depending_secondary()
    {
        return this->template await_secondary<SecondaryAssetHandlerT>(
            asset_path());
    }

    auto & await_depending_primary()
    {
        return this->await(this->primary_asset_async(asset_path()));
    }

public:
    explicit SingleDependencySecondaryAssetHandler(std::string asset_path)
        : mAssetPath(std::move(asset_path))
    {
    }

    using BaseT = SingleDependencySecondaryAssetHandler;

protected:
    void append_features(SecondaryAssetHandlerBase::Hasher &hasher) override
    {
        hasher.append(mAssetPath);
    }
};
}
