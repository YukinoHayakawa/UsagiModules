#pragma once

#include <random>

#include <Usagi/Runtime/Service/ServiceAccess.hpp>

namespace usagi
{
struct ServiceRandomNumberGenerator
{
    using ServiceT = std::mt19937;

    static ServiceT & get_service()
    {
        // todo how to reproduce random sequence?
        thread_local static std::mt19937 rng { std::random_device{}() };
        return rng;
    }
};
}
USAGI_DECL_SERVICE_ALIAS(usagi::ServiceRandomNumberGenerator, rng);
