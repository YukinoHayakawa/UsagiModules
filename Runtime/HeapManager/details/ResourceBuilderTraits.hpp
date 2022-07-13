#pragma once

#include <tuple>

#include "../TransparentArg.hpp"

namespace usagi::details::heap_manager
{
template <typename T>
struct BuildArgTraits
{
    // type used to match value hash function
    using HashT = std::remove_cvref_t<T>;
    // type of value that will be stored in ResourceBuildTask, in tuple
    // using StorageT = std::remove_reference_t<T>;
    // type of value that will be received by construct function
    using InvokeT = T;

    template <typename Received>
    static decltype(auto) hash_forward_or_convert(Received &&arg)
    {
        // if the requester passes a value of exactly the same type,
        // pass through.
        if constexpr(std::is_same_v<std::remove_cvref_t<Received>, HashT>)
        {
            return std::forward<Received>(arg);
        }
        // otherwise perform conversion to ensure correct hash behavior
        else
        {
            return static_cast<HashT>(arg);
        }
    }
};

// Specialization for transparent arguments.
template <typename T>
struct BuildArgTraits<TransparentArg<T>>
{
    using HashT = TransparentArg<std::remove_cvref_t<T>>;
    // using StorageT = std::remove_cvref_t<T>;
    using InvokeT = T;

    template <typename Received>
    static HashT hash_forward_or_convert(Received &&)
    {
        // ignore the value
        return { };
    }
};

template <typename T>
struct InvocationArgumentTupleT;

template <typename... Args>
struct InvocationArgumentTupleT<std::tuple<Args...>>
{
    using TupleT = std::tuple<typename BuildArgTraits<Args>::InvokeT...>;
};

template <typename Builder>
struct ResourceBuilderTraits
{
    // Types of build arguments the builder declares to accept.
    using DeclaredArgumentTupleT = typename Builder::BuildArguments;
    // using ArgumentStorageTupleT = 
    // Types of values that will be used to determine the identity of the
    // resource. Values returned by the build arg func will be converted to the
    // types declared in this tuple type before passed to the hasher.
    // using CanonicalArgumentTupleT =
    // typename CanonicalizeHashArguments<DeclaredArgumentTupleT>::TupleT;
    using InvocationArgumentTupleT = 
        typename InvocationArgumentTupleT<DeclaredArgumentTupleT>::TupleT;
};

template <typename Builder, std::size_t I, typename Arg>
decltype(auto) hash_forward_or_convert(Arg &&arg)
{
    using DeclaredArgT = std::tuple_element_t<
        I,
        typename ResourceBuilderTraits<Builder>::DeclaredArgumentTupleT
    >;
    using ArgTraits = BuildArgTraits<DeclaredArgT>;

    return ArgTraits::hash_forward_or_convert(std::forward<Arg>(arg));
}
}
