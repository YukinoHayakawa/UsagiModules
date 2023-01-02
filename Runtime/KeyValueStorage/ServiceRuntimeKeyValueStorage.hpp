#pragma once

#include <map>
#include <string>
#include <variant>
#include <Usagi/Entity/detail/EntityId.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Runtime/Service/ServiceAccess.hpp>
#include <Usagi/Runtime/Service/SimpleService.hpp>

namespace usagi
{
// todo thread safety
struct RuntimeKeyValueStorage
{
    std::map<std::string, std::variant<
        std::string,
        EntityId,
        double,
        float,
        std::uint64_t,
        std::int64_t,
        std::int32_t,
        bool
    >, std::less<>> values;

    template <typename Val>
    Val & require(const std::string_view key)
    {
        const auto iter = values.find(key);
        USAGI_ASSERT_THROW(
            iter != values.end(),
            std::runtime_error("key not found")
        );
        // may throw std::bad_variant_access
        return std::get<Val>(iter->second);
    }

    template <typename Val>
    Val & ensure(const std::string_view key)
    {
        auto iter = values.find(key);
        if(iter != values.end())
        {
            USAGI_ASSERT_THROW(
                std::holds_alternative<Val>(iter->second),
                std::runtime_error("unmatched value type")
            );
        }
        else
        {
            iter = values.emplace(key, Val { }).first;
        }
        return std::get<Val>(iter->second);
    }
};

using ServiceRuntimeKeyValueStorage = SimpleService<RuntimeKeyValueStorage>;
}

USAGI_DECL_SERVICE_ALIAS(usagi::ServiceRuntimeKeyValueStorage, kv_storage);
