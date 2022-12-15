#pragma once

#include <Usagi/Library/Utilities/Variant.hpp>

#include "ScheduleNodeProcessorReady.hpp"

namespace usagi
{
template <typename Graph>
struct ScheduleMixinNodeVisitorAvailableProcessors
{
    auto available_processors()
    {
        const auto graph = [this] { return static_cast<Graph *>(this); };

        // find out idle processors
        // todo: optimize time complexity
        const auto filter = [](auto &&idx_v_pair) {
            return std::visit(Overloaded {
                // ReSharper disable once CppParameterMayBeConstPtrOrRef
                [](ScheduleNodeProcessorReady &n) {
                    return n.occupied == false;
                },
                [](auto &&) { return false; }
            }, std::get<1>(idx_v_pair));
        };

        return graph()->filtered_vertices(filter) |
            graph()->template func_get_vertices_as<
                ScheduleNodeProcessorReady
            >();
    }
};
}
