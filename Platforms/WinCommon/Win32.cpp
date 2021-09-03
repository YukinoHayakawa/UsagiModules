#include "Win32.hpp"

#include <fmt/format.h>

#include <Usagi/Runtime/IO/Unicode.hpp>

namespace usagi
{
// https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/using-ntstatus-values
// NTSTATUS value ref:
// https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
void check_nt_status(std::u8string_view function, const NTSTATUS status)
{
    if(NT_SUCCESS(status)) return;

    if(NT_INFORMATION(status))
    {
        fmt::print(
            "{}: NT_INFORMATION {:#x}\n",
            u8str_to_char(function),
            (ULONG)status
        );
    }
    else if(NT_WARNING(status))
    {
        fmt::print(
            "{}: NT_WARNING {:#x}\n",
            u8str_to_char(function),
            (ULONG)status
        );
    }
    else
    {
        USAGI_THROW(NtException(function, status));
    }
}
}
