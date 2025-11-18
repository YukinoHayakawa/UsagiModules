#pragma once

#include <cstdint>

/*
 * This file defines equivalent common types appears in Quirrel so you don't
 * have to include the bigger headers of the scripting language.
 */
namespace usagi::scripting::quirrel::types
{
// Numerical types

using sq_int_t    = std::int64_t;
using sq_uint_t   = std::uint64_t;
using sq_int32_t  = std::int32_t;
using sq_uint32_t = std::uint32_t;
using sq_int64_t  = std::int64_t;
using sq_uint64_t = std::uint64_t;
using sq_float_t  = double;
using sq_bool_t   = sq_uint_t;
using sq_char_t   = char;

template <typename T>
struct limits;

template <>
struct limits<sq_char_t>
{
    constexpr static sq_char_t max_v = '\xff';
    static_assert(max_v == static_cast<char>(255));
};

// Internal types

using hash_t           = std::uint64_t;
using raw_object_val_t = sq_uint_t;
using user_pointer_t   = void *;
using result_t         = sq_int_t;

// tagSQMessageSeverity
enum class MessageLevels
{
    Info    = 0,
    Warning = 1,
    Error   = 2,
};
} // namespace usagi::scripting::quirrel::types

// Something like TEXT(str). Gotta be for Windows.
#ifndef _SC
#define _SC(str) str
#endif
