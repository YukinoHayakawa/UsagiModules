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

template <typename ResourceBuilderT>
class ResourceBuildTask : public ResourceBuildTaskBase
{
    using AccessorT = ResourceAccessor<ResourceBuilderT>;
    using ContextT = ResourceBuildContext<ResourceBuilderT>;
    using ProductT = typename ResourceBuilderT::ProductT;

    ContextT *mContext = nullptr;
    // Notified by ResourceBuildTask after constructing the resource, whether
    // the resource building was successful or failed.
    std::promise<void> mPromise;
    ResourceBuilderT mBuilder;

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
        ContextT *context,
        std::promise<void> promise,
        Args &&...args)
        : mContext(context)
        , mPromise(std::move(promise))
        , mBuilder(std::forward<Args>(args)...)
    {
    }

    bool precondition() override
    {
        return mContext->entry->state.load() == ResourceState::SCHEDULED;
    }

    void on_started() override
    {
        set_state(ResourceState::BUILDING);
    }

    void run() override
    {
        try
        {
            ResourceConstructDelegate<ResourceBuilderT> delegate(mContext);
            set_state(mBuilder.construct(delegate));
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
        return true;
    }
};
}
