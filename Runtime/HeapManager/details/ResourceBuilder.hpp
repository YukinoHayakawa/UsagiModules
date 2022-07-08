#pragma once

#include <concepts>

#include <Usagi/Runtime/ReturnValue.hpp>

#include "ResourceConstructDelegate.hpp"

namespace usagi
{
namespace details
{
// instantiation of this derived class is used to test whether a resource
// builder accepts the set of parameters it declares.
template <typename Builder>
struct ResourceBuilderParameterValidationHelper : private Builder
{
    using ConstructDelegate =
        ResourceConstructDelegate<typename Builder::ProductT>;

    template <typename Tuple, std::size_t... I>
    auto do_validate_construct_params(
        ConstructDelegate &delegate,
        Tuple &&t,
        std::index_sequence<I...>)
    {
        return this->construct(delegate, std::get<I>(t)...);
    }

    template <typename Tuple>
    auto validate_construct_params(
        ConstructDelegate &delegate,
        Tuple &&t)
    {
        return do_validate_construct_params(
            delegate,
            t,
            std::make_index_sequence<std::tuple_size_v<Tuple>>()
        );
    }
};

template <typename T, typename Tuple>
concept ResourceConstructibleFromTuple = requires(
    ResourceBuilderParameterValidationHelper<T> helper)
{
    {
        helper.validate_construct_params(
            std::declval<ResourceConstructDelegate<typename T::ProductT> &>(),
            std::declval<typename T::BuildArguments>()
        )
    } -> std::same_as<ResourceState>;
};
}

template <typename T>
concept ResourceBuilder = requires(T t)
{
    // ensure the product type
    typename T::ProductT;

    // ensure build argument type declaration. the types decide how the
    // resource id is generated.
    typename T::BuildArguments;

    // ensure that the construct method can be called with declared argument
    // types
    requires details::ResourceConstructibleFromTuple<
        T,
        typename T::BuildArguments
    >;
};

namespace static_tests
{
struct RbTest1
{
    using ProductT = int;
    using BuildArguments = std::tuple<int, bool>;
    ResourceState construct(ResourceConstructDelegate<int> &, int, bool);
};
static_assert(ResourceBuilder<RbTest1>);

/*
struct RbTest2
{
    using ProductT = int;
    using BuildArguments = std::tuple<int, bool>;
    ResourceState construct(ResourceConstructDelegate<bool> &, int);
};
static_assert(!ResourceBuilder<RbTest2>);
*/
}
}
