﻿#pragma once

// #include <Usagi/Library/Utilities/Functional.hpp>

namespace usagi
{
template <typename ProductT>
ResourceConstructDelegate<ProductT>::ResourceConstructDelegate(
    ContextT *context)
    : mContext(context)
{
}

template <typename ProductT>
template <typename... Args>
ProductT & ResourceConstructDelegate<ProductT>::emplace(Args &&...args)
    requires std::is_constructible_v<ProductT, Args &&...>
{
    const auto entry = mContext->entry;
    assert(!entry->payload.has_value());
    assert(!entry->deleter);
    return entry->payload.emplace(std::forward<Args>(args)...);
}

template <typename ProductT>
void ResourceConstructDelegate<ProductT>::set_deleter(
    std::function<void()> deleter)
{
    const auto entry = mContext->entry;
    assert(entry->payload.has_value());
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
    Args &&... build_args)
-> ResourceAccessor<typename AnotherBuilderT::ProductT>
{
    /*return resource_apply<AnotherBuilderT>(
        // std::forward_as_tuple(std::forward<Args>(build_params)...)
        // todo: how to handle rvalues properly?
        std::make_tuple(std::forward<Args>(build_params)...)
    );*/

    auto requester = mContext->manager->template resource<AnotherBuilderT>(
        { },
        mContext->executor,
        // safe to forward as tuple here because the lambda has no local vars
        [&] { return std::forward_as_tuple(std::forward<Args>(build_args)...); }
        // only used once here so forward it
        // [&] { return std::forward<ArgTuple>(args_tuple); }
    );

    // create the dependency edge
    requester.requesting_from(mContext->entry->descriptor);
    // try to rebuild the resource since it is now requested
    requester.rebuild_if_failed();
    requester.rebuild_if_evicted();

    return requester.make_request();
}

template <typename ProductT>
template <typename AnotherBuilderT, typename... Args>
auto ResourceConstructDelegate<ProductT>::resource_transient(
    Args &&... build_args)
-> ResourceAccessor<typename AnotherBuilderT::ProductT>
{
    return mContext->manager->template resource_transient<AnotherBuilderT>(
        std::forward<Args>(build_args)...);
    // return resource_apply<AnotherBuilderT, true>(
        // std::forward_as_tuple(std::forward<Args>(build_params)...)
        // std::make_tuple(std::forward<Args>(build_params)...)
    // );
}

template <typename ProductT>
HeapResourceDescriptor
ResourceConstructDelegate<ProductT>::make_unique_descriptor() const
{
    return mContext->manager->make_unique_descriptor();
}

/*
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
            // only used once here so forward it
            [&] { return std::forward<ArgTuple>(args_tuple); }
        );

        // create the dependency edge
        requester.requesting_from(mContext->entry->descriptor);
        // try to rebuild the resource since it is now requested
        requester.rebuild_if_failed();
        requester.rebuild_if_evicted();

        return requester.make_request();
    }
}
*/

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
