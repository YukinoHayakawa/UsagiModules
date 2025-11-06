#pragma once

#include <string>

namespace usagi
{
struct ServiceDatabaseInterfaceProvider
{
    using ServiceT = ServiceDatabaseInterfaceProvider;

    ServiceT & get_service()
    {
        return *this;
    }

    std::string dbi_pch_src_asset_path;
    std::string dbi_pch_bin_asset_path;
};
}
