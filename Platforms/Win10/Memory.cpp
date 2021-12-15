#include <Usagi/Modules/Platforms/WinCommon/Win32.hpp>
#include <Usagi/Runtime/Platform/Memory.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi::platform::memory
{
void prefetch(void *ptr, const std::size_t size_bytes)
{
    WIN32_MEMORY_RANGE_ENTRY entry;

    entry.VirtualAddress = ptr;
    entry.NumberOfBytes = size_bytes;

    // Windows 8
    USAGI_WIN32_CHECK_THROW(
        PrefetchVirtualMemory,
        NtCurrentProcess(),
        1,
        &entry,
        0
    );
}

MemoryView offer(void *ptr, std::size_t size_bytes)
{
    // Windows 8.1 Update
    USAGI_WIN32_CHECK_THROW(
        OfferVirtualMemory,
        ptr, size_bytes, VmOfferPriorityVeryLow
    );

    return { ptr, size_bytes };
}
}
