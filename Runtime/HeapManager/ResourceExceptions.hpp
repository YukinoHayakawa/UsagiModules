#pragma once

#include <stdexcept>

namespace usagi
{
class ResourceException : public std::runtime_error
{
public:
    ResourceException() : std::runtime_error("") { }
};

class ResourceNotReady final : public ResourceException { };
}
