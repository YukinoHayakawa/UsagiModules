#pragma once

#include <type_traits>

namespace usagi
{
// https://en.wikipedia.org/wiki/Linear_interpolation#Programming_language_support
template <typename T>
constexpr T lerp(T a, T b, T t)
    requires std::is_floating_point_v<T>
{
    return (1 - t) * a + t * b;
}

// https://www.gamedev.net/articles/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/
template <typename T>
constexpr T inverse_lerp(T a, T b, T value)
    requires std::is_floating_point_v<T>
{
    return (value - a) / (b - a);
}

template <typename T>
constexpr T remap(T a, T b, T c, T d, T v)
    requires std::is_floating_point_v<T>
{
    return lerp(c, d, inverse_lerp(a, b, v));
}
}
