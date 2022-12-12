#pragma once

#include <cassert>
#include <type_traits>

#include "ScheduleMixinRootNodeIndex.hpp"
#include "ScheduleNodeRoot.hpp"

namespace usagi
{
struct ScheduleModifierInsertRoot
{
    template <typename Graph>
    void operator()(Graph &&graph, float ready_time = 0) requires
        std::is_base_of_v<
            ScheduleMixinRootNodeIndex<
                typename std::decay_t<Graph>::VertexIndexT>,
            std::decay_t<Graph>
        >
    {
        assert(graph.root_node_index == -1);
        auto [idx, ref] = graph.template add_vertex<ScheduleNodeRoot>();
        ref.ready_time = ready_time;
        graph.root_node_index = idx;
        graph.root_node_ptr = &ref;
    }
};
}
