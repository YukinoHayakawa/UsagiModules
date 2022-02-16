#pragma once

#include <Usagi/Library/Utilities/ArgumentStorage.hpp>
#include <Usagi/Modules/IO/Windowing/NativeWindow.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>
#include <Usagi/Modules/Runtime/HeapManager/TransparentArg.hpp>

#include "HeapWindowManager.hpp"

namespace usagi
{
class RbNativeWindow
    : ArgumentStorage<
        std::uint64_t,                      // identifier
        TransparentArg<std::string_view>,   // default title
        TransparentArg<Vector2f>,           // default position
        TransparentArg<Vector2f>,           // default size
        TransparentArg<float>,              // default dpi scaling
        TransparentArg<NativeWindowState>   // default state
    >
{
public:
    using ArgumentStorage::ArgumentStorage;

    using TargetHeapT = HeapWindowManager;
    using ProductT = NativeWindow;

    ResourceState construct(
        ResourceConstructDelegate<RbNativeWindow> &delegate);
};
}
