#include <Usagi/Module/Platform/WinCommon/Win32.hpp>
#include <Usagi/Runtime/Platform/Memory.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

namespace usagi::platform::memory
{
void prefetch(void *ptr, const std::size_t size_bytes)
{
    WIN32_MEMORY_RANGE_ENTRY entry;

    entry.VirtualAddress = ptr;
    entry.NumberOfBytes = size_bytes;

    if(!PrefetchVirtualMemory(
        NtCurrentProcess(),
        1,
        &entry,
        0
    )) WIN32_THROW("PrefetchVirtualMemory");
}

MemoryRegion offer(void *ptr, std::size_t size_bytes)
{
    if(!OfferVirtualMemory(ptr, size_bytes, VmOfferPriorityVeryLow))
        WIN32_THROW("OfferVirtualMemory");

    MemoryRegion region;

    region.base_address = ptr;
    region.length = size_bytes;

    return region;
}
}
