#pragma once

#include <cassert>

#include <Usagi/Runtime/Task/Task.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "ResourceAccessor.hpp"
#include "ResourceConstructDelegate.hpp"

namespace usagi
{
template <typename ResourceBuilderT, typename BuildArgTuple>
class ResourceBuildTask final : public Task
{
    using ProductT = typename ResourceBuilderT::ProductT;
    using AccessorT = ResourceAccessor<ProductT>;
    using UniqueContextT = std::unique_ptr<
        ResourceBuildContext<ProductT>,
        details::heap_manager::RequestContextDeleter
    >;

    UniqueContextT mContext;
    // Notified by ResourceBuildTask after constructing the resource, whether
    // the resource building was successful or failed.
    std::promise<void> mPromise;
    // The tuple that will be expanded to call the construct function of the
    // resource builder. The tuple can only contain rvalue refs when requesting
    // a transient resource.
    BuildArgTuple mArguments;

    ResourceState get_state() const
    {
        return mContext->entry->state.load();
    }

    void set_state(const ResourceState state)
    {
        mContext->entry->state.store(state);
    }

public:
    template <typename... Args>
    ResourceBuildTask(
        // acquired from ResourceRequestHandler
        UniqueContextT context,
        std::promise<void> promise,
        // args may be used to construct the tuple, or it may be a tuple to
        // be moved from.
        Args &&...args)
        : mContext(std::move(context))
        , mPromise(std::move(promise))
        , mArguments(std::forward<Args>(args)...)
    {
    }

    bool precondition() override
    {
        const auto a = get_state() == ResourceState::SCHEDULED;
        const auto b = mContext->entry->payload.has_value() == false;
        // will be used when requesting other resources
        const auto c = mContext->executor != nullptr;

        return a && b && c;
    }

    void on_started() override
    {
        set_state(ResourceState::BUILDING);
    }

    void run() override try
    {
        ResourceConstructDelegate<ProductT> delegate(mContext.get());
        ResourceBuilderT builder;

        // Note that rvalues in the build arg tuple may be moved by this
        // invocation.
        const auto state = std::apply([&]<typename...Args>(Args &&...args) {
            return builder.construct(delegate, std::forward<Args>(args)...);
        }, mArguments);

        set_state(state);
    }
    catch(const std::runtime_error &e)
    {
        LOG(error, "[Heap] Resource {} failed to build: {}",
            mContext->entry->descriptor, e.what());
        set_state(ResourceState::FAILED);
    }

    void on_finished() override
    {
        mPromise.set_value();
    }

    bool postcondition() override
    {
        return get_state().failed() || mContext->entry->payload.has_value();
    }
};
}
