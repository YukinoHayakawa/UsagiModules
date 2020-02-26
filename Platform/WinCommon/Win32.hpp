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

#include <stdexcept>

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
}

#define NT_CHECK_THROW(function) \
    if(!NT_SUCCESS(status)) \
        USAGI_THROW(NtException(u8##function, status)) \
/**/

#define WIN32_THROW(function) \
    USAGI_THROW(Win32Exception(u8##function)) \
/**/
