﻿#include <cassert>

#include <Usagi/Runtime/Platform/File.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

#include "Win32.hpp"

namespace usagi::platform::file
{
NativeFileHandle open(
    const std::filesystem::path &path,
    const FileOpenMode mode,
    const FileOpenOptions options)
{
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
        path.native().c_str(),
        desired_access,
        FILE_SHARE_READ,
        nullptr,
        creation,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if(handle == INVALID_HANDLE_VALUE)
        USAGI_WIN32_THROW("CreateFileW");

    return handle;
}

NativeFileHandle open_exception_translated(
    const std::filesystem::path &path,
    FileOpenMode mode,
    FileOpenOptions options)
{
    try
    {
        return open(path, mode, options);
    }
    catch(const win32::Win32Exception &e)
    {
        switch(e.error_code)
        {
            case ERROR_FILE_NOT_FOUND:
                throw ExceptionFileNotFound(e.what());
            case ERROR_ACCESS_DENIED:
                throw ExceptionFileNotFound(e.what());
            case ERROR_SHARING_VIOLATION:
                throw ExceptionFileNotFound(e.what());
            default:
                throw;
        }
    }
}

void close(NativeFileHandle file)
{
    USAGI_WIN32_CHECK_THROW(CloseHandle, file);
}

std::size_t size(NativeFileHandle file)
{
    LARGE_INTEGER li;
    USAGI_WIN32_CHECK_THROW(GetFileSizeEx, file, &li);
    return li.QuadPart;
}

std::uint64_t id(NativeFileHandle file)
{
    BY_HANDLE_FILE_INFORMATION info { };
    USAGI_WIN32_CHECK_THROW(GetFileInformationByHandle, file, &info);
    return win32::make_u64(info.nFileIndexHigh, info.nFileIndexLow);
}

std::uint64_t last_modification_time(NativeFileHandle file)
{
    BY_HANDLE_FILE_INFORMATION info { };
    USAGI_WIN32_CHECK_THROW(GetFileInformationByHandle, file, &info);
    return win32::make_u64(
        info.ftLastWriteTime.dwHighDateTime,
        info.ftLastWriteTime.dwLowDateTime
    );
}

void replace_file(
    const std::filesystem::path &replaced_file,
    const std::filesystem::path &replacement_file,
    bool backup,
    const std::filesystem::path &backup_name)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew

    USAGI_WIN32_CHECK_THROW(
        ReplaceFileW,
        replaced_file.native().c_str(),
        replacement_file.native().c_str(),
        backup ? backup_name.native().c_str() : nullptr,
        0, nullptr, nullptr
    );
}

namespace
{
void nt_unmap_section(NativeFileMappingObject &internal)
{
    assert(internal.base_address);

    const auto status = NtUnmapViewOfSection(
        NtCurrentProcess(),
        internal.base_address
    );
    USAGI_NT_CHECK_THROW("NtUnmapViewOfSection");
    internal.base_address = nullptr;
}

void nt_close_section(NativeFileMappingObject &internal)
{
    assert(internal.section_handle);

    const auto status = NtClose(internal.section_handle);
    USAGI_NT_CHECK_THROW("NtClose");
    internal.section_handle = nullptr;
}

auto nt_map(MemoryMapping &mapping, std::uint64_t length, std::uint64_t commit)
{
    // Windows virtual address allocation is 64K-aligned, otherwise
    // NtMapViewOfSection returns STATUS_MAPPED_ALIGNMENT.
    // https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223

    const auto offset = mapping.internal.offset;
    const auto protect = mapping.internal.protect;
    const auto map_offset_remainder = offset % 0x10000; // 64K aligned
    const auto map_offset_round_down = offset - map_offset_remainder;
    const auto map_length = map_offset_remainder + length;

    void *view_base = nullptr;
    LARGE_INTEGER map_offset;
    map_offset.QuadPart = map_offset_round_down;
    SIZE_T view_size = map_length;

    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmapviewofsection
    const auto status = NtMapViewOfSection(
        mapping.internal.section_handle,
        NtCurrentProcess(),
        &view_base,
        0,
        map_offset_remainder + commit,
        &map_offset,
        &view_size,
        ViewShare,
        // bug: do not use MEM_RESERVE if the mapped file is not supposed to be modified
        // note that MEM_RESERVE requires PAGE_READWRITE (see ntoskrnl source).
        // violation of it causes 0xC000004E STATUS_SECTION_PROTECTION.
        MEM_RESERVE,
        protect
    );

    if(NT_SUCCESS(status))
    {
        mapping.internal.base_address = view_base;
        mapping.heap = {
            static_cast<char*>(view_base) + map_offset_remainder,
            length
        };
    }

    return status;
}
}

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntcreatesection
// https://doxygen.reactos.org/de/d40/filemap_8c.html#ab537c934fd29d080f1a28c9d72e0ea4a
// https://devblogs.microsoft.com/oldnewthing/20080227-00/?p=23303
// https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory
// https://github.com/reactos/reactos/blob/893a3c9d030fd8b078cbd747eeefd3f6ce57e560/dll/win32/kernel32/client/file/filemap.c#L45
// bug: cannot create read-only mappings
// bug: can't create view of empty file
MemoryMapping map(
    NativeFileHandle file,
    MemoryMappingMode mode,
    std::uint64_t offset,
    std::uint64_t length,
    std::uint64_t initial_commit)
{
    // https://docs.microsoft.com/en-us/windows/win32/memory/memory-protection-constants
    DWORD protect = 0;
    HANDLE section;
    ACCESS_MASK access =
        STANDARD_RIGHTS_REQUIRED |
        SECTION_QUERY |
        SECTION_MAP_READ |
        SECTION_EXTEND_SIZE |
        0;

    if(mode & MAPPING_READ)
    {
        protect = PAGE_READONLY;
    }
    if(mode & MAPPING_WRITE)
    {
        protect = PAGE_READWRITE;
        access |= SECTION_MAP_WRITE;
    }

    LARGE_INTEGER sec_size;
    sec_size.QuadPart = length + offset;

    NTSTATUS status = NtCreateSection(
        &section,
        access,
        nullptr,
        &sec_size,
        protect,
        SEC_RESERVE,
        file
    );
    USAGI_NT_CHECK_THROW("NtCreateSection");

    // The actual mapping is larger but only the requested part is returned
    MemoryMapping mapping;
    mapping.internal.section_handle = section;
    mapping.internal.offset = offset;
    mapping.internal.protect = protect;

    status = nt_map(mapping, length, initial_commit);

    if(!NT_SUCCESS(status))
        NtClose(section);

    USAGI_NT_CHECK_THROW("NtMapViewOfSection");

    return mapping;
}

void remap(MemoryMapping &mapping, std::uint64_t new_size)
{
    // release the old mapping
    nt_unmap_section(mapping.internal);

    // extend the section
    LARGE_INTEGER sec_size;
    sec_size.QuadPart = mapping.internal.offset + new_size;
    auto status = NtExtendSection(
        mapping.internal.section_handle,
        &sec_size
    );
    USAGI_NT_CHECK_THROW("NtExtendSection");

    // create a new mapping
    status = nt_map(mapping, new_size, 0);
    USAGI_NT_CHECK_THROW("NtMapViewOfSection");
}

void unmap(MemoryMapping &mapping)
{
    nt_unmap_section(mapping.internal);
    nt_close_section(mapping.internal);
}
}
