#pragma once

#include <Usagi/Library/Memory/Noncopyable.hpp>

namespace usagi
{
// Common base class for misc resources.
class Resource : Noncopyable
{
public:
    virtual ~Resource() = default;
};
}
