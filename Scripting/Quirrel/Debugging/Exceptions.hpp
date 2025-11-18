#pragma once

#include <Usagi/Runtime/Exceptions/Exceptions.hpp>

namespace usagi::scripting::quirrel::debugging
{
class VirtualMachineHasNoDebuggingInterface : public runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;

    explicit VirtualMachineHasNoDebuggingInterface(void * vm)
        : RuntimeError(
              "Quirrel VM {} does not implement `DebuggingInterface`.", vm
          )
    {
    }
};
} // namespace usagi::scripting::quirrel::debugging
