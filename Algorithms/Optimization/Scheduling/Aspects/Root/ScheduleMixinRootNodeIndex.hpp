#pragma once

#include "ScheduleNodeRoot.hpp"

namespace usagi
{
template <typename VertexIndexT>
struct ScheduleMixinRootNodeIndex
{
    VertexIndexT root_node_index = -1;
    ScheduleNodeRoot *root_node_ptr = nullptr;
};
}
