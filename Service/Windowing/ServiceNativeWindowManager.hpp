#pragma once

#include "NativeWindowManager.hpp"

namespace usagi
{
struct ServiceNativeWindowManager
{
    using ServiceType = NativeWindowManager;

    ServiceType & get_service()
    {
        return *manager.get();
    }

    std::unique_ptr<NativeWindowManager> manager;

    explicit ServiceNativeWindowManager(
        std::unique_ptr<NativeWindowManager> manager)
        : manager(std::move(manager))
    {
    }
};
}
