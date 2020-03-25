#pragma once

#include <Usagi/Module/Common/Math/Matrix.hpp>
#include <Usagi/Game/Entity/Component.hpp>

namespace usagi
{
class ComponentPosition
{
public:
    Vector3f position = { 0, 0, 0 };
};
static_assert(Component<ComponentPosition>);
}
