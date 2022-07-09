﻿#pragma once

#include <Usagi/Library/Memory/Noncopyable.hpp>
#include <Usagi/Library/Memory/Nonmovable.hpp>

#include "HeapResourceDescriptor.hpp"
#include "ResourceAccessor.hpp"
#include "ResourceRequestContext.hpp"

namespace usagi
{
class TaskExecutor;
class HeapManager;

template <typename SrcHeap, typename SrcRes, typename DstHeap, typename DstRes>
struct HeapTransfer;

template <typename Product>
class ResourceConstructDelegate : Noncopyable, Nonmovable
{
public:
    using ProductT = Product;
    using ContextT = ResourceBuildContext<ProductT>;

private:
    ContextT *mContext = nullptr;

public:
    explicit ResourceConstructDelegate(ContextT *context);

    // ********************************************************************* //    
    //                           Resource Allocation                         //
    // ********************************************************************* //

    void emplace(ProductT product, std::function<void()> deleter = []{});

    template <typename HeapT>
    HeapT * heap();

    // ********************************************************************* //    
    //                            Resource Request                           //
    // ********************************************************************* //

    template <typename AnotherBuilderT, typename... Args>
    [[nodiscard]]
    auto resource(Args &&...build_params)
        -> ResourceAccessor<typename AnotherBuilderT::ProductT>;

    template <typename AnotherBuilderT, typename... Args>
    [[nodiscard]]
    auto resource_transient(Args &&...build_params)
        -> ResourceAccessor<typename AnotherBuilderT::ProductT>;

    template <
        typename AnotherBuilderT,
        bool Transient = false,
        typename ArgTuple
    >
    [[nodiscard]]
    auto resource_apply(ArgTuple &&args_tuple)
        -> ResourceAccessor<typename AnotherBuilderT::ProductT>;

    // request a descriptor that is unique from heap manager. used for
    // requesting transient resources.
    HeapResourceDescriptor make_unique_descriptor() const;

    // ********************************************************************* //    
    //                           Resource Transfer                           //
    // ********************************************************************* //

    /*
    template <
        typename AnotherBuilderT,
        typename... Args
    >
    [[nodiscard]]
    auto transfer(ProductT &dst_res, Args &&...args);
    */
};
}
