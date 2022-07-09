#pragma once

// Include this header when implementing resource builders that don't request
// resources from the HeapManager.

#include <Usagi/Runtime/ReturnValue.hpp>

#include "details/ResourceConstructDelegate.hpp"
#include "details/ResourceConstructDelegateImpl.hpp"
#include "TransparentArg.hpp"

namespace usagi
{
/*template <typename ProductT>
using ResourceConstructResult = ReturnValue<
    ResourceState,
    ProductT
>;

template <typename Builder, typename Product>
class ResourceBuilderBase
{
public:
    using ProductT = Product;

    virtual ~ResourceBuilderBase() = default;

    // force the derived class to implement the method. not actually supposed
    // to be virtual.
    virtual ResourceConstructResult<ProductT> construct(
        ResourceConstructDelegate<Builder> &delegate) = 0;
};*/

template <typename Product, typename... BuildArgs>
class ResourceBuilderDecl
{
public:
    using ProductT = Product;
    using BuildArguments = std::tuple<std::remove_reference_t<BuildArgs>...>;

    virtual ~ResourceBuilderDecl() = default;

    virtual ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        BuildArgs... args) = 0;
};
}
