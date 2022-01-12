#include "AssetEnum.hpp"

#include <Usagi/Library/Utility/EnumTranslation.hpp>

namespace usagi
{
template <>
struct EnumMapping<AssetPriority, std::string_view>
{
    constexpr static auto MAPPING = {
        std::pair { AssetPriority::GAMEPLAY_CRITICAL, "Critical" },
        std::pair { AssetPriority::LOD_STREAMING, "Streaming" },
        std::pair { AssetPriority::ENV_AMBIENT, "Ambient" },
    };
};

std::string_view to_string(AssetPriority val)
{
    return EnumAcceptor<AssetPriority, std::string_view>(val);
}

template <>
struct EnumMapping<AssetStatus, std::string_view>
{
    constexpr static auto MAPPING = {
        std::pair { AssetStatus::MISSING, "Missing" },
        std::pair { AssetStatus::EXIST, "Exist" },
        std::pair { AssetStatus::EXIST_BUSY, "Exist, Busy" },
        std::pair { AssetStatus::QUEUED, "Queued" },
        std::pair { AssetStatus::LOADING, "Loading" },
        std::pair { AssetStatus::READY, "Ready" },
        std::pair { AssetStatus::FAILED, "Failed" },
        std::pair { AssetStatus::OUTDATED, "Outdated" },
        std::pair { AssetStatus::MISSING_DEPENDENCY, "Missing Dependency" },
    };
};

std::string_view to_string(AssetStatus val)
{
    return EnumAcceptor<AssetStatus, std::string_view>(val);
}
}
