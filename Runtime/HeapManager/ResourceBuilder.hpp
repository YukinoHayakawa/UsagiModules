#pragma once

// Include this header when implementing resource builders that don't request
// resources from the HeapManager.

#include "details/ResourceConstructDelegate.hpp"
#include "details/ResourceConstructDelegateImpl.hpp"
#include "TransparentArg.hpp"

namespace usagi
{
namespace details::heap_manager
{
template <typename T>
struct BuildArgTrait
{
    using HashT = std::remove_cvref_t<T>;
    using ArgumentT = T;
};

template <typename T>
struct BuildArgTrait<TransparentArg<T>>
{
    using HashT = TransparentArg<std::remove_cvref_t<T>>;
    using ArgumentT = T;
};
}

template <typename Product, typename... BuildArgs>
class ResourceBuilderDecl
{
    template <typename T>
    using ArgHashT = 
        typename details::heap_manager::BuildArgTrait<T>::HashT;

    template <typename T>
    using ArgInvokeT = 
        typename details::heap_manager::BuildArgTrait<T>::ArgumentT;

public:
    using ProductT = Product;
    using BuildArguments = std::tuple<ArgHashT<BuildArgs>...>;

    virtual ~ResourceBuilderDecl() = default;
    
    // force the derived class to implement the method. not actually supposed
    // to be virtual.
    virtual ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        ArgInvokeT<BuildArgs>...) = 0;
};
}
