#pragma once

namespace usagi
{
template <typename ResourceBuilderT>
ResourceConstructDelegate<ResourceBuilderT>::ResourceConstructDelegate(
    ContextT *context)
    : mContext(context)
{
}

template <typename ResourceBuilderT>
ResourceConstructDelegate<ResourceBuilderT>::~ResourceConstructDelegate()
{
    USAGI_ASSERT_THROW(
        mObjectAllocated,
        std::runtime_error("Resource builder didn't allocate any object!")
    );
}

template <typename ResourceBuilderT>
template <typename ArgTuple>
decltype(auto) ResourceConstructDelegate<ResourceBuilderT>::allocate_apply(
    ArgTuple &&args_tuple)
{
    return USAGI_APPLY(allocate, args_tuple);
}

template <typename ResourceBuilderT>
template <typename ... Args>
decltype(auto) ResourceConstructDelegate<ResourceBuilderT>::allocate(
    Args &&... args)
{
    // todo stronger test?
    assert(!mObjectAllocated);

    if constexpr(HeapHasTemplatedAllocateFunc<
        TargetHeapT, ProductT, Args...>)
    {
        decltype(auto) ret = mContext->heap->template allocate<ProductT>(
            mContext->entry->descriptor.resource_id(),
            std::forward<Args>(args)...
        );
        mObjectAllocated = true;
        return ret;
    }
    else
    {
        decltype(auto) ret = mContext->heap->allocate(
            mContext->entry->descriptor.resource_id(),
            std::forward<Args>(args)...
        );
        mObjectAllocated = true;
        return ret;
    }

    // decltype(auto) ret = mContext->heap->allocate(
    //     Tag<ProductT>(),
    //     mContext->entry->descriptor.resource_id(),
    //     std::forward<Args>(args)...
    // );
    // mObjectAllocated = true;
    //
    // return ret;
}

template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ResourceBuilderT>::resource(
    Args &&...build_params)
-> ResourceAccessor<AnotherBuilderT>
{
    return resource_apply<AnotherBuilderT>(
        std::forward_as_tuple(std::forward<Args>(build_params)...)
    );
}

template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename ArgTuple>
auto ResourceConstructDelegate<ResourceBuilderT>::resource_apply(
    ArgTuple &&args_tuple)
-> ResourceAccessor<AnotherBuilderT>
{
    auto requester = mContext->manager->template resource<AnotherBuilderT>(
        { },
        mContext->executor,
        [&] { return args_tuple; }
    );
    requester.requesting_from(mContext->entry->descriptor);
    requester.rebuild_if_failed();
    requester.rebuild_if_evicted();
    // Requesting resource from a resource builder always results in evaluation
    // of the parameters. So it doesn't matter if there are rvalue refs in
    // the parameters.
    return requester.make_request();
}

template <typename ResourceBuilderT>
template <typename AnotherBuilderT, typename ... Args>
auto ResourceConstructDelegate<ResourceBuilderT>::transfer(
    ProductT &dst_res,
    Args &&... args)
{
    auto src_accessor = resource<AnotherBuilderT>(
        std::forward<Args>(args)...
    );
    auto src_heap = src_accessor.mHeap;
    decltype(auto) src_res = src_accessor.await();

    // the transfer function may interact with the heaps to accomplish the
    // operation. e.g. gpu copy
    HeapTransfer<
        typename AnotherBuilderT::TargetHeapT,
        typename AnotherBuilderT::ProductT,
        TargetHeapT,
        ProductT
    > transfer_func;

    return transfer_func(
        *src_heap,
        src_res.cref(),
        *mContext->heap,
        dst_res
    );
}
}
