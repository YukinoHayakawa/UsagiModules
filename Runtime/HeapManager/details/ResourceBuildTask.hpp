#pragma once

#include <future>
#include <cassert>

#include <Usagi/Runtime/Task.hpp>

#include "ResourceAccessor.hpp"
#include "ResourceConstructDelegate.hpp"

namespace usagi
{
class ResourceBuildTaskBase : public Task
{
protected:
    HeapManager *mManager = nullptr;
    TaskExecutor *mExecutor = nullptr;
    std::promise<void> mPromise;

public:
    ResourceBuildTaskBase(
        HeapManager *manager,
        TaskExecutor *executor,
        std::promise<void> promise)
        : mManager(manager)
        , mExecutor(executor)
        , mPromise(std::move(promise))
    {
    }
};

template <typename ResourceBuilderT>
class ResourceBuildTask : public ResourceBuildTaskBase
{
    using AccessorT = ResourceAccessor<ResourceBuilderT>;

    AccessorT mAccessor;
    ResourceBuilderT mBuilder;

public:
    template <typename... Args>
    ResourceBuildTask(
        HeapManager *manager,
        TaskExecutor *executor,
        AccessorT accessor,
        std::promise<void> promise,
        Args &&...args)
        : ResourceBuildTaskBase(manager, executor, std::move(promise))
        , mAccessor(std::move(accessor))
        , mBuilder(std::forward<Args>(args)...)
    {
        assert(mAccessor.is_fallback() == false);
    }

    bool precondition() override
    {
        return mAccessor.mEntry->state.load() == ResourceState::SCHEDULED;
    }

    void on_started() override
    {
        mAccessor.mEntry->state.store(ResourceState::BUILDING);
    }

    void run() override
    {
        ResourceConstructDelegate<ResourceBuilderT> delegate(
            mAccessor.descriptor(),
            mManager,
            mAccessor.mHeap,
            mExecutor
        );
        mBuilder.construct(delegate);
        // mAccessor.mEntry->state.store(delegate.state());
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
