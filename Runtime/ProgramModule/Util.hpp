#pragma once

#include <string>
#include <string_view>
#include <typeinfo>

namespace usagi
{
std::string demangle_typeid(const std::type_info &ti);
std::string demangle(std::string_view mangled);
}
