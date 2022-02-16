#include "HeapWindowManager.hpp"

#include <Usagi/Library/Utilities/StringView.hpp>

namespace usagi
{
NativeWindow & HeapWindowManager::allocate(
    HeapResourceIdT id,
    std::uint64_t identifier,
    std::string_view default_title,
    const Vector2f &default_position,
    const Vector2f &default_size,
    float default_dpi_scaling,
    NativeWindowState default_state)
{
    // todo unify interface
    return *mManager->create_window(
        to_string_view(id),
        default_title,
        default_position,
        default_size,
        default_dpi_scaling,
        default_state
    );
}

NativeWindow & HeapWindowManager::resource(HeapResourceIdT id)
{
    return *mManager->window(to_string_view(id));
}
}
