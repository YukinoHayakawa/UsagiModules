#include <filesystem>

#include <Usagi/Runtime/Platform/File.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

#include "Win32.hpp"

namespace usagi::platform::file
{
NativeFileHandle open(
    const std::u8string_view path,
    const FileOpenMode mode,
    const FileOpenOptions options)
{
    const std::filesystem::path native_path(path);

    DWORD desired_access = 0;
    DWORD creation = OPEN_EXISTING;

    if(mode & OPEN_READ)
        desired_access |= GENERIC_READ;
    if(mode & OPEN_WRITE)
        desired_access |= GENERIC_WRITE;

    if(options & OPTION_CREATE_IF_MISSING)
        creation = OPEN_ALWAYS;
    if(options & OPTION_ALWAYS_CREATE_NEW)
        creation = CREATE_ALWAYS;

    const auto handle = CreateFileW(
        native_path.native().c_str(),
        desired_access,
        FILE_SHARE_READ,
        nullptr,
        creation,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if(handle == INVALID_HANDLE_VALUE)
        WIN32_THROW("CreateFileW");

    return handle;
}

void close(const NativeFileHandle file)
{
    if(!CloseHandle(file))
        WIN32_THROW("CloseHandle");
}

std::size_t size(const NativeFileHandle file)
{
    LARGE_INTEGER li;

    if(!GetFileSizeEx(file, &li))
        WIN32_THROW("GetFileSizeEx");

    return li.QuadPart;
}

std::size_t read_at(
    NativeFileHandle file,
    std::size_t offset,
    void *buf,
    std::size_t size)
{
    // todo impl
    USAGI_THROW(int());
}

std::size_t write_at(
    NativeFileHandle file,
    std::size_t offset,
    const void *buf,
    std::size_t size)
{
    // todo impl
    USAGI_THROW(int());
}


// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatesection
// https://doxygen.reactos.org/de/d40/filemap_8c.html#ab537c934fd29d080f1a28c9d72e0ea4a
// https://devblogs.microsoft.com/oldnewthing/20080227-00/?p=23303
// https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory
MemoryMapping map(
    const NativeFileHandle file,
    const MemoryMappingMode mode,
    const std::size_t max_size)
{
    DWORD protect = 0;

    if(mode & MAPPING_READ)
        protect = PAGE_READONLY;
    if(mode & MAPPING_WRITE)
        protect = PAGE_READWRITE;

    LARGE_INTEGER size;
    size.QuadPart = max_size;

    const HANDLE section = CreateFileMappingW(
        file,
        nullptr,
        protect,
        size.HighPart,
        size.LowPart,
        nullptr
    );

    if(!section)
        WIN32_THROW("CreateFileMappingW");

    void *view_base = nullptr;
    SIZE_T view_size = 0;

    const NTSTATUS status = NtMapViewOfSection(
        section,
        NtCurrentProcess(),
        &view_base,
        0, 0, nullptr,
        &view_size,
        ViewShare,
        0,
        protect
    );

    if(!NT_SUCCESS(status))
        NtClose(section);

    NT_CHECK_THROW("NtMapViewOfSection");

    MemoryMapping mapping;
    mapping.mapping = section;
    mapping.heap.base_address = view_base;
    mapping.heap.length = view_size;

    return mapping;
}

void unmap(const MemoryMapping &mapping)
{
    auto status = NtUnmapViewOfSection(
        NtCurrentProcess(),
        mapping.heap.base_address
    );

    NT_CHECK_THROW("NtUnmapViewOfSection");

    status = NtClose(mapping.mapping);

    NT_CHECK_THROW("NtClose");
}
}
