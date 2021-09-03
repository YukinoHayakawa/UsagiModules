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

    USAGI_WIN32_CHECK_THROW(
        PrefetchVirtualMemory,
        NtCurrentProcess(),
        1,
        &entry,
        0
    );
}

MemoryRegion offer(void *ptr, std::size_t size_bytes)
{
    USAGI_WIN32_CHECK_THROW(
        OfferVirtualMemory,
        ptr, size_bytes, VmOfferPriorityVeryLow
    );

    MemoryRegion region;

    region.base_address = ptr;
    region.length = size_bytes;

    return region;
}
}
