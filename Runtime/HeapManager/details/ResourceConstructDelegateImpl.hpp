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
void ResourceConstructDelegate<ProductT>::emplace(
    ProductT product,
    std::function<void()> deleter)
{
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
-> ResourceAccessor<typename AnotherBuilderT::ProductT>
{
    return resource_apply<AnotherBuilderT>(
        std::forward_as_tuple(std::forward<Args>(build_params)...)
    );
}

template <typename ProductT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ProductT>::resource_transient(
    Args &&...build_params)
-> ResourceAccessor<typename AnotherBuilderT::ProductT>
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
-> ResourceAccessor<typename AnotherBuilderT::ProductT>
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
