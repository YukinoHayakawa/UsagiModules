#pragma once

#include <concepts>

#include "ResourceConstructDelegate.hpp"

namespace usagi
{
template <typename T>
concept ResourceBuilder = requires(T t)
{
    // The builder must be able to specify the heap it will construct the
    // target resource on. It may obtain the heap id from the build parameters
    // passed in by the user.
    { t.target_heap() } -> std::same_as<std::uint64_t>;
    typename T::TargetHeapT;
    typename T::ProductT;
    { t.construct(std::declval<ResourceConstructDelegate<T> &>()) };
        // -> std::same_as<typename T::ProductT &>;
};
}
