#pragma once

#include <memory>

#include "ResourceBuildOptions.hpp"

namespace usagi
{
class HeapManager;
class TaskExecutor;

struct ResourceBuildContextCommon
    // Prevent accidental copying.
    : Noncopyable
{
    // <Set during allocation>

    // Block id inside the pool allocator. Set when allocated.
    std::uint64_t context_index = 0;

    HeapManager *manager = nullptr;

    // <Set during request()>

    // May be used when a resource builder requests another resource.
    TaskExecutor *executor = nullptr;
};

template <typename Product>
struct ResourceBuildContext : ResourceBuildContextCommon
{
    using ProductT = Product;

    ResourceEntry<ProductT> *entry = nullptr;
};

template <typename ResourceBuilderT, typename BuildArgFuncT>
struct ResourceRequestContext
    : ResourceBuildContext<typename ResourceBuilderT::ProductT>
{
    // Only used in ResourceRequestHandler
    ResourceBuildOptions options;

    // The function that returns a tuple of arguments passed to ResourceBuilder
    // when invoked. This ref only have to be valid until
    // HeapManager.resource() returns. That's the only opportunity of
    // evaluating the builder arguments. todo The arguments will be directly
    // forwarded to the builder constructor. 
    const BuildArgFuncT *arg_func = nullptr;
};

namespace details::heap_manager
{
/**
 * \brief Used to evaluate the size of ResourceRequestContext
 */
struct DummyResourceBuilder
{
    using ProductT = void;
};

// Implemented in HeapManager.cpp
struct RequestContextDeleter
{
    void operator()(const ResourceBuildContextCommon *context) const;
};
}

/**
 * \brief Type-erased memory block used to store resource request context.
 * Every instantiations of ResourceRequestContext have the same size.
 */
union ResourceRequestContextBlock
{
    ResourceRequestContext<
        details::heap_manager::DummyResourceBuilder,
        int
    > context;
    char bytes[sizeof(decltype(context))] { };
};

template <typename Builder, typename LazyBuildArgFunc>
using UniqueResourceRequestContext = std::unique_ptr<
    ResourceRequestContext<Builder, LazyBuildArgFunc>,
    details::heap_manager::RequestContextDeleter
>;
}
