#pragma once

namespace usagi
{
template <typename ProductT>
ResourceConstructDelegate<ProductT>::ResourceConstructDelegate(
    ContextT *context)
    : mContext(context)
{
}

template <typename ProductT>
ResourceConstructDelegate<ProductT>::~ResourceConstructDelegate()
{
    // todo shouldn't throw in dtor
    // checked in ResourceBuildTask::postcondition
    /*USAGI_ASSERT_THROW(
        mObjectAllocated,
        std::runtime_error("Resource builder didn't allocate any object!")
    );*/
}

/*
template <typename ProductT>
template <typename ArgTuple>
decltype(auto) ResourceConstructDelegate<ProductT>::allocate_apply(
    ArgTuple &&args_tuple)
{
    return USAGI_APPLY(allocate, args_tuple);
}

template <typename ProductT>
template <typename ... Args>
decltype(auto) ResourceConstructDelegate<ProductT>::allocate(
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
*/

template <typename ProductT>
void ResourceConstructDelegate<ProductT>::emplace(
    ProductT product,
    std::function<void()> deleter)
{
    // assert(!mObjectAllocated);
    const auto entry = mContext->entry;
    assert(!entry->payload.has_value());
    entry->payload.emplace(std::move(product));
    // todo add a default deleter
    entry->deleter = std::move(deleter);
}

template <typename ProductT>
template <typename HeapT>
HeapT * ResourceConstructDelegate<ProductT>::heap()
{
    return mContext->manager->template locate_heap<HeapT>();
}

template <typename ProductT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ProductT>::resource(
    Args &&...build_params)
-> ResourceAccessor<AnotherBuilderT>
{
    return resource_apply<AnotherBuilderT>(
        std::forward_as_tuple(std::forward<Args>(build_params)...)
    );
}

template <typename ProductT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ProductT>::resource_transient(
    Args &&...build_params)
-> ResourceAccessor<AnotherBuilderT>
{
    return resource_apply<AnotherBuilderT, true>(
        std::forward_as_tuple(std::forward<Args>(build_params)...)
    );
}

template <typename ProductT>
HeapResourceDescriptor
ResourceConstructDelegate<ProductT>::make_unique_descriptor() const
{
    return mContext->manager->make_unique_descriptor();
}

template <typename ProductT>
template <typename AnotherBuilderT, bool Transient, typename ArgTuple>
auto ResourceConstructDelegate<ProductT>::resource_apply(
    ArgTuple &&args_tuple)
-> ResourceAccessor<AnotherBuilderT>
{
    // Requesting resource from a resource builder always results in evaluation
    // of the parameters. So it doesn't matter if there are rvalue refs in
    // the parameters.
    if constexpr(Transient)
    {
        return USAGI_APPLY(
            mContext->manager->template resource_transient<AnotherBuilderT>,
            args_tuple
        );
    }
    else
    {
        auto requester = mContext->manager->template resource<AnotherBuilderT>(
            { },
            mContext->executor,
            [&] { return args_tuple; }
        );
        // create the dependency edge
        requester.requesting_from(mContext->entry->descriptor);
        // try to rebuild the resource since it is now requested
        requester.rebuild_if_failed();
        requester.rebuild_if_evicted();
        return requester.make_request();
    }
}

/*
template <typename ProductT>
template <typename AnotherBuilderT, typename ... Args>
auto ResourceConstructDelegate<ProductT>::transfer(
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
*/
}
