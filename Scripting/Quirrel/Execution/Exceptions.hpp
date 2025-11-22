#pragma once

#include <Usagi/Runtime/Exceptions/Exceptions.hpp>

namespace usagi::scripting::quirrel
{
class InvalidVirtualMachine : public runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;

    InvalidVirtualMachine() : RuntimeError("Encountered a null Quirrel VM.") {}
};

class MismatchedObjectType : public runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;
};

class ScriptCompilationError : public runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;
};

class ScriptExecutionError : public runtime::RuntimeError
{
public:
    using RuntimeError::RuntimeError;
};
} // namespace usagi::scripting::quirrel
