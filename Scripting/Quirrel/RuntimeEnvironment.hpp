#pragma once

#include <Usagi/Library/Meta/Reflection/StaticReflection.hpp>
#include <Usagi/Runtime/Services/SimpleServiceProvider.hpp>

namespace usagi::scripting::quirrel
{
constexpr std::meta::info service_provider_t =
    ^^usagi::runtime::SimpleServiceProvider;

struct RuntimeEnvironment
{
    [:service_provider_t:] service_provider;
};
} // namespace usagi::scripting::quirrel
