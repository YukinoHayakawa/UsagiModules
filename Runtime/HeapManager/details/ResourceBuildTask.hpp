#pragma once

#include <cassert>

#include <Usagi/Runtime/Task/Task.hpp>

#include "ResourceAccessor.hpp"
#include "ResourceConstructDelegate.hpp"

namespace usagi
{
class ResourceBuildTaskBase : public Task
{
public:
    // todo this is a temporary solution
    virtual void set_executor(TaskExecutor *executor) = 0;
};

// todo maybe the build arguments should be stored in build task instead of forcing each resource builder to impl its own
template <typename ResourceBuilderT>
class ResourceBuildTask : public ResourceBuildTaskBase
{
    using AccessorT = ResourceAccessor<ResourceBuilderT>;
    using ProductT = typename ResourceBuilderT::ProductT;
    using BuildArguments = typename ResourceBuilderT::BuildArguments;
    using ContextT = std::unique_ptr<
        ResourceBuildContext<ProductT>,
        details::heap_manager::RequestContextDeleter
    >;

    ContextT mContext;
    // Notified by ResourceBuildTask after constructing the resource, whether
    // the resource building was successful or failed.
    std::promise<void> mPromise;
    // ResourceBuilderT mBuilder;
    BuildArguments mArguments;

    ResourceState get_state() const
    {
        return mContext->entry->state.load();
    }

    void set_state(const ResourceState state)
    {
        mContext->entry->state.store(state);
    }

    void set_executor(TaskExecutor *executor) override
    {
        mContext->executor = executor;
    }

public:
    template <typename... Args>
    ResourceBuildTask(
        ContextT context,
        std::promise<void> promise,
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

        return a && b;
    }

    void on_started() override
    {
        set_state(ResourceState::BUILDING);
    }

    void run() override
    {
        try
        {
            ResourceConstructDelegate<ProductT> delegate(mContext.get());
            ResourceBuilderT builder;
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
