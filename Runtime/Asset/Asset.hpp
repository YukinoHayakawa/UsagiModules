#pragma once

#include <Usagi/Library/Memory/Noncopyable.hpp>

namespace usagi
{
/*
 * Base class for assets.
 */
class Asset : Noncopyable
{
public:
    virtual ~Asset() = default;
};
}
