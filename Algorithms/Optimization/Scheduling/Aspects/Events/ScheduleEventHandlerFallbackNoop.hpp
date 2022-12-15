#pragma once

namespace usagi
{
template <typename VertexIndexT>
struct ScheduleEventHandlerFallbackNoop
{
    static void on_vertex_added(VertexIndexT index, auto &&v_ref)
    {
        // no-op on purpose
    }

    static void on_edge_added(
        VertexIndexT from_idx,
        auto &&from_v,
        VertexIndexT to_idx,
        auto &&to_v)
    {
        // no-op on purpose
    }
};
}
