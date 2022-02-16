#pragma once

#include "HeapManager.hpp"

namespace usagi
{
// todo this impl is temp
template <typename... HeapTypes>
class HeapManagerStatic : HeapManager
{
public:
    template <typename... ArgTuples>
    HeapManagerStatic(ArgTuples &&... arg_tuples)
    {
        (..., USAGI_APPLY(add_heap<HeapTypes>, arg_tuples));
    }

    using HeapManager::resource_immediate;
    using HeapManager::resource;
    using HeapManager::request_resource;

    template <typename HeapT>
    HeapT * locate_heap() requires requires (HeapT h)
    { std::get<HeapT>(std::declval<std::tuple<HeapTypes...>>()); }
    {
        return HeapManager::locate_heap<HeapT>();
    }
};
}
