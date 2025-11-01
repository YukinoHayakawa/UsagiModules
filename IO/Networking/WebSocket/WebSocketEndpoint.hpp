#pragma once

#include <string>

namespace cavia::network
{
// Shio: Represents a full WebSocket endpoint with address, port, and path.
struct FullEndpoint
{
    std::string    address;
    unsigned short port;
    std::string    path;
};
} // namespace cavia::network
