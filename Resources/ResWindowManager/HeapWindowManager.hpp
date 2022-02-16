#pragma once

#include <Usagi/Modules/IO/Windowing/NativeWindowManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/Heap.hpp>
#include <Usagi/Modules/Runtime/HeapManager/details/HeapResourceDescriptor.hpp>

namespace usagi
{
class HeapWindowManager : public Heap
{
    std::unique_ptr<NativeWindowManager> mManager =
        NativeWindowManager::create_native_manager();

public:
    NativeWindow & allocate(
        HeapResourceIdT id,
        std::uint64_t identifier,
        std::string_view default_title,
        const Vector2f &default_position,
        const Vector2f &default_size,
        float default_dpi_scaling,
        NativeWindowState default_state
    );

    NativeWindow & resource(HeapResourceIdT id);
};
}
