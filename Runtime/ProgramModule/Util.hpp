#pragma once

#include <string>
#include <string_view>

namespace usagi
{
std::string demangle(std::string_view mangled);
}
