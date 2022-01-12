#pragma once

namespace usagi
{
class TaskExecutor;

template <typename ResourceBuilderT>
class ResourceConstructDelegate
{
public:
    using TargetHeapT = typename ResourceBuilderT::TargetHeapT;
    using ProductT = typename ResourceBuilderT::ProductT;

private:
    HeapResourceDescriptor mDescriptor;
    HeapManager *mManager = nullptr;
    TargetHeapT *mHeap = nullptr;
    TaskExecutor *mExecutor = nullptr;

public:
    ResourceConstructDelegate(
        HeapResourceDescriptor descriptor,
        HeapManager *manager,
        TargetHeapT *heap,
        TaskExecutor *executor)
        : mDescriptor(std::move(descriptor))
        , mManager(manager)
        , mHeap(heap)
        , mExecutor(executor)
    {
    }

    template <typename... Args>
    decltype(auto) allocate(Args &&...args)
    {
        return mHeap->allocate(
            mDescriptor.resource_id(),
            std::forward<Args>(args)...
        );
    }

    template <
        typename AnotherBuilderT,
        typename... Args
    >
    auto resource(Args &&...build_params);
};
}
