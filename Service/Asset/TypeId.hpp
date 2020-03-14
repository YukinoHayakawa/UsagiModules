#pragma once

namespace usagi
{
template <typename T>
struct TypeId
{
    static inline int dummy;
};

template <typename T>
constexpr void *TYPE_ID = &TypeId<T>::dummy;
}
