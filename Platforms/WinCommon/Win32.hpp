#pragma once

/*
 * This file includes the Windows header and undef any macros conflicting with
 * our codes.
 */

#pragma warning(disable: 4005) // macro redefinition

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#define DEVICE_TYPE DWORD
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#include "ntos.h"

#ifdef DELETE
#   undef DELETE
#endif

#ifdef near
#   undef near
#endif

#ifdef far
#   undef far
#endif

#ifdef FAR
#   undef FAR
#endif

#ifdef NEAR
#   undef NEAR
#endif

#include <cassert>
#include <stdexcept>
#include <string_view>

#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi
{
struct NtException : std::runtime_error
{
    const std::u8string_view function;
    const NTSTATUS status;

    NtException(std::u8string_view function, const NTSTATUS status)
        : runtime_error("Nt function call failed.")
        , function(std::move(function))
        , status(status)
    {
    }
};

struct Win32Exception : std::runtime_error
{
    const std::u8string_view function;
    const DWORD error_code;

    explicit Win32Exception(std::u8string_view function)
        : runtime_error("Win32 function call failed.")
        , function(std::move(function))
        , error_code(GetLastError())
    {
    }
};

void check_nt_status(std::u8string_view function, NTSTATUS status);
}

#define USAGI_NT_CHECK_THROW(function) ::usagi::check_nt_status(u8##function, status)

#define USAGI_WIN32_THROW(function) \
    USAGI_THROW(Win32Exception(u8##function)) \
/**/

#define USAGI_WIN32_CHECK_THROW(function, ...) \
    do { if(!function(__VA_ARGS__)) \
        USAGI_WIN32_THROW(#function); } while(false) \
/**/

#define USAGI_WIN32_CHECK_ASSERT(function, ...) \
    do { [[maybe_unused]] \
        const auto ret = function(__VA_ARGS__); \
        assert(ret); } while(false) \
/**/

#define USAGI_WIN32_CHECK_ASSIGN_THROW(var, function, ...) \
    do { if(!(var = function(__VA_ARGS__))) \
        USAGI_THROW(Win32Exception(u8## #function)); } while(false) \
/**/
