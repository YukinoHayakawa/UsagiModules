#include <Usagi/Runtime/Platform/Memory.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>
#include <Usagi/Library/Math/Rounding.hpp>

#include "Win32.hpp"

namespace usagi::platform::memory
{
std::size_t page_size()
{
    static std::size_t size = 0;
    if(size == 0)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        size = sys_info.dwPageSize;
    }
    return size;
}

std::size_t round_up_to_page_size(const std::size_t size_bytes)
{
    return round_up_unsigned(size_bytes, page_size());
}

std::size_t round_down_to_page_size(const std::size_t size_bytes)
{
    return round_down_unsigned(size_bytes, page_size());
}

// For memory reservation and committing, see:
// https://docs.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory

MemoryRegion allocate(const std::size_t size_bytes, const bool commit)
{
    MemoryRegion ret;
    ret.length = size_bytes;

    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntallocatevirtualmemory
    const auto status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &ret.base_address,
        0,
        &ret.length,
        commit ? MEM_RESERVE | MEM_COMMIT : MEM_RESERVE,
        commit ? PAGE_READWRITE : PAGE_NOACCESS
    );

    USAGI_NT_CHECK_THROW("NtAllocateVirtualMemory");

    return ret;
}

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntallocatevirtualmemory
MemoryRegion commit(void *ptr, std::size_t size_bytes)
{
    const auto status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &ptr,
        0,
        &size_bytes,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    USAGI_NT_CHECK_THROW("NtAllocateVirtualMemory");

    return { ptr, size_bytes };
}

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntfreevirtualmemory
MemoryRegion decommit(void *ptr, std::size_t size_bytes)
{
    const auto status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &ptr,
        &size_bytes,
        MEM_DECOMMIT
    );

    USAGI_NT_CHECK_THROW("NtFreeVirtualMemory");

    return { ptr, size_bytes };
}

MemoryRegion free(void *ptr, std::size_t size_bytes)
{
    const auto status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &ptr,
        &size_bytes,
        MEM_RELEASE
    );

    USAGI_NT_CHECK_THROW("NtFreeVirtualMemory");

    return { ptr, size_bytes };
}

// Doc: https://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FMemory%20Management%2FVirtual%20Memory%2FNtLockVirtualMemory.html

MemoryRegion lock(void *ptr, std::size_t size_bytes)
{
    const auto status = NtLockVirtualMemory(
        NtCurrentProcess(),
        &ptr,
        &size_bytes,
        0x0001 // This is used, when calling KERNEL32.DLL VirtualLock routine
    );

    USAGI_NT_CHECK_THROW("NtLockVirtualMemory");

    return { ptr, size_bytes };
}

// Doc: https://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FMemory%20Management%2FVirtual%20Memory%2FNtUnlockVirtualMemory.html

MemoryRegion unlock(void *ptr, std::size_t size_bytes)
{
    const auto status = NtUnlockVirtualMemory(
        NtCurrentProcess(),
        &ptr,
        &size_bytes,
        0x0001 // This is used, when calling KERNEL32.DLL VirtualLock routine
    );

    USAGI_NT_CHECK_THROW("NtUnlockVirtualMemory");

    return { ptr, size_bytes };
}

// offer() and prefetch() rely on API added in Win8.1, therefore not included
// here. Link with corresponding lib such as Win7/Win10.

// https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile
MemoryRegion flush(void *ptr, std::size_t size_bytes)
{
    USAGI_WIN32_CHECK_THROW(FlushViewOfFile, ptr, size_bytes);
    return { ptr, size_bytes };
}

MemoryRegion zero_pages(void *ptr, std::size_t size_bytes)
{
    // Decommitting and recommitting memory will zero the pages.
    // See: https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualallocex#mem_reset
    decommit(ptr, size_bytes);
    return commit(ptr, size_bytes);
}
}
