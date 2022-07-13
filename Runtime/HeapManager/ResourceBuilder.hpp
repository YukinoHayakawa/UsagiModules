#pragma once

// Include this header when implementing resource builders that don't request
// resources from the HeapManager.

#include "details/ResourceConstructDelegate.hpp"
#include "details/ResourceConstructDelegateImpl.hpp"
#include "details/ResourceBuilderTraits.hpp"

namespace usagi
{
/*
 * dev notes:
 * build task should store copied values
 * lazy arg func should not return refs pointing to vars inside the lambda
 * but if the tuple is constructed from invoking params they can be forwarded
 * transient resource: forward values from caller to builder without copying
 */

/**
 * \brief Helper class used for defining the interface of a resource builder.
 * \tparam Product The type of resource that will be finally stored in
 * ResourceEntry and accessed by requester.
 * \tparam BuildArgs The type of arguments that will be finally passed to
 * construct function. Wrap the type in TransparentArg if it should not be
 * used to determine the identity of the resource.
 */
template <typename Product, typename... BuildArgs>
class ResourceBuilderDecl  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    using ProductT = Product;
    using BuildArguments = std::tuple<BuildArgs...>;

    virtual ~ResourceBuilderDecl() = default;
    
    // force the derived class to implement the method. not actually supposed
    // to be virtual.
    virtual ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        typename details::heap_manager::BuildArgTraits<BuildArgs>::InvokeT...)
    = 0;
};
}
