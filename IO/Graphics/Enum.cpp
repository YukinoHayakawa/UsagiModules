#include "Enum.hpp"

#include <Usagi/Library/Utilities/EnumTranslation.hpp>

namespace usagi
{
template <>
struct EnumMapping<GpuShaderStage, std::string_view>
{
    constexpr static auto MAPPING = {
        std::pair { GpuShaderStage::VERTEX, "Vertex" },
        std::pair { GpuShaderStage::FRAGMENT, "Fragment" },
    };
};

std::string_view to_string(GpuShaderStage val)
{
    return EnumAcceptor<GpuShaderStage, std::string_view>(val);
}
}
