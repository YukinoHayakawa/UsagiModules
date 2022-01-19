#pragma once

#include <vector>

#include <Usagi/Modules/Runtime/HeapManager/Resource.hpp>

namespace usagi
{
class SpirvBytecodes : public Resource
{
    std::vector<std::uint32_t> mBytecodes;

public:
    explicit SpirvBytecodes(std::vector<std::uint32_t> bytecodes)
        : mBytecodes(std::move(bytecodes))
    {
    }

    const std::vector<std::uint32_t> & bytecodes() const
    {
        return mBytecodes;
    }
};
}
