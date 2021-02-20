#pragma once

#include <memory>

#include "InputEventSource.hpp"

namespace usagi
{
struct ServiceInputSource
{
    using ServiceType = InputEventSource;

    ServiceType & get_service()
    {
        return *mEventPump;
    }

    explicit ServiceInputSource(std::unique_ptr<InputEventSource> event_pump)
        : mEventPump(std::move(event_pump))
    {
    }

private:
    std::unique_ptr<InputEventSource> mEventPump;
};
}
