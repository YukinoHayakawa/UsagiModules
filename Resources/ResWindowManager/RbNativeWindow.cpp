#include "RbNativeWindow.hpp"

#include <Usagi/Modules/IO/Windowing/NativeWindowManager.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

namespace usagi
{
ResourceState RbNativeWindow::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    std::string_view identifier,
    std::string_view default_title,
    const Vector2f &default_position,
    const Vector2f &default_size,
    float default_dpi_scaling,
    NativeWindowState default_state)
{
    const auto heap = delegate.heap<NativeWindowManager>();

    auto window = heap->create_window(
        // identifier,
        default_title,
        default_position,
        default_size,
        default_dpi_scaling,
        default_state
    );

    delegate.emplace(std::move(window));

    return ResourceState::READY;
}
}
