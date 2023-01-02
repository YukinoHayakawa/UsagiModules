#pragma once

#include <fixed_string.hpp>

#include <Usagi/Runtime/Service/ServiceAccess.hpp>

#include "ServiceRuntimeKeyValueStorage.hpp"

namespace usagi
{
template <fixstr::fixed_string Key, typename Value>
struct OperatorReadRuntimeKeyValue
{
    using ServiceAccessT = ServiceAccess<ServiceRuntimeKeyValueStorage>;

    Value operator()(ServiceAccessT &rt, auto &&db)
    {
        return rt.kv_storage().require<Value>(Key);
    }
};
}
