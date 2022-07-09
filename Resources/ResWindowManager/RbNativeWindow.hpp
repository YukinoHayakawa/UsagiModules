#pragma once

#include <Usagi/Modules/Common/Math/Matrix.hpp>
#include <Usagi/Modules/IO/Windowing/NativeWindowState.hpp>
#include <Usagi/Modules/Runtime/HeapManager/ResourceBuilder.hpp>

namespace usagi
{
class NativeWindow;

class RbNativeWindow : public ResourceBuilderDecl<
    std::shared_ptr<NativeWindow>,
    std::string_view,                   // identifier
    TransparentArg<std::string_view>,   // default title
    TransparentArg<const Vector2f &>,   // default position
    TransparentArg<const Vector2f &>,   // default size
    TransparentArg<float>,              // default dpi scaling
    TransparentArg<NativeWindowState>   // default state
>
{
public:
    ResourceState construct(
        ResourceConstructDelegate<ProductT> &delegate,
        std::string_view identifier,
        std::string_view default_title,
        const Vector2f &default_position,
        const Vector2f &default_size,
        float default_dpi_scaling,
        NativeWindowState default_state) override;
};
}
