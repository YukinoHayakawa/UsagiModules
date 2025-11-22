#pragma once

#include <Usagi/Library/Meta/Reflection/Enums.hpp>
#include <Usagi/Runtime/Exceptions/Exceptions.hpp>
#include <Usagi/Runtime/Memory/Safety/MemoryAccessErrorCodes.hpp>

namespace usagi::scripting::quirrel::runtime
{
class VirtualMachineAccessViolation : public usagi::runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;

    void * invalid_address = nullptr;

    using error_codes_t = usagi::runtime::memory::MemoryAccessErrorCodes;

    error_codes_t error_code;

    VirtualMachineAccessViolation(
        void * vm, void * addr, const error_codes_t code
    )
        : RuntimeError(
              "Quirrel VM {} is trying to access invalid address {}, resulted "
              "in error code {}.",
              vm,
              addr,
              meta::reflection::enum_to_string(code)
          )
        , invalid_address(addr)
        , error_code(code)
    {
    }
};
} // namespace usagi::scripting::quirrel::runtime
