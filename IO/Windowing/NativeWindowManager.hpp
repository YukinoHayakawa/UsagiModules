#pragma once

#include <Usagi/Modules/Common/Math/Matrix.hpp>

#include "NativeWindowState.hpp"

namespace usagi
{
class NativeWindow;

class NativeWindowManager
{
public:
    virtual ~NativeWindowManager() = default;

    // todo use some kind of other id instead of string view? u64? uuid? note that using uuid won't affect game correctness because it's not participated in computations.
    virtual std::shared_ptr<NativeWindow> create_window(
        // std::string_view identifier,
        std::string_view title,
        const Vector2f &position,
        const Vector2f &size,
        float dpi_scaling,
        NativeWindowState state
    ) = 0;
    // virtual void destroy_window(std::string_view identifier) = 0;

    // virtual NativeWindow * window(std::string_view identifier) = 0;
    // virtual void destroy_unused_windows() = 0;

    // implemented in platform modules.
    static std::unique_ptr<NativeWindowManager> create_native_manager();

    // virtual Vector2u32 currentDisplayResolution() = 0;

    /**
     * \brief Read window events from system event queue and dispatch
     * them to corresponding window instances.
     */
    // virtual void process_events() = 0;
};
}
