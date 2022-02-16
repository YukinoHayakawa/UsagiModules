#include "RbNativeWindow.hpp"

namespace usagi
{
ResourceState RbNativeWindow::construct(
    ResourceConstructDelegate<RbNativeWindow> &delegate)
{
    delegate.allocate_apply(arguments());

    return ResourceState::READY;
}
}
