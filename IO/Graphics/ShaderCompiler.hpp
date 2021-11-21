#pragma once

#include <string_view>
#include <vector>

#include "Enum.hpp"

namespace usagi::spirv
{
std::vector<std::uint32_t> from_glsl(
    std::string_view source,
    GpuShaderStage stage);
}
