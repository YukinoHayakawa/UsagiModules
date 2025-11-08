#pragma once

#include <type_traits>

#include <sqconfig.h>
#include <squirrel.h>

namespace usagi::scripting::quirrel::types
{
static_assert(_SQ64, "Why on earth would you compile a 32-bit program?");

// Numerical types

using Int32 = SQInt32;
static_assert(std::is_same_v<SQInt32, std::int32_t>);
using UInt32 = SQUnsignedInteger32;
static_assert(std::is_same_v<SQUnsignedInteger32, std::uint32_t>);
using Int64 = SQInteger;
static_assert(std::is_same_v<SQInteger, std::int64_t>);
using UInt64 = SQUnsignedInteger;
static_assert(std::is_same_v<SQUnsignedInteger, std::uint64_t>);
using Double = SQFloat;
static_assert(std::is_same_v<SQFloat, double>);
using Bool = SQUnsignedInteger;
static_assert(std::is_same_v<Bool, SQUnsignedInteger>);
using Char = SQChar;
static_assert(std::is_same_v<Char, char>);

// Internal types

using Hash = SQHash;
static_assert(std::is_same_v<SQHash, std::uint64_t>);
using RawObjectVal = SQRawObjectVal;
static_assert(std::is_same_v<SQRawObjectVal, SQUnsignedInteger>);
using UserPointer = SQUserPointer;
static_assert(std::is_same_v<SQUserPointer, void *>);
using OperationResult = SQRESULT;
static_assert(std::is_same_v<SQRESULT, SQInteger>);

using ExecutionContext = HSQUIRRELVM;
using Handle           = HSQOBJECT;
} // namespace usagi::scripting::quirrel::types
