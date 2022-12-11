#pragma once

#include <map>
#include <variant>
#include <deque>
#include <cassert>

namespace usagi
{
/**
 * \brief The constraint graph mixes task graph & processor timeline together.
 * todo: optimize space & time complexities.
 * \tparam EventHandler The class that handles vertex & edge events. It should
 * inherit this base class as per CRTP idiom.
 * \tparam EnabledNodes Allowed type of vertices.
 */
template <
    typename EventHandler,
    typename... EnabledNodes // put in std::variant?
>
struct ScheduleConstraintGraph
{
    using VertexT = std::variant<EnabledNodes...>;
    using VertexIndexT = std::uint64_t;

    // deque is used to keep references valid
    std::deque<VertexT> vertices;
    std::multimap<VertexIndexT, VertexIndexT> edges;

    template <typename Vertex>
    Vertex & vertex(VertexIndexT index)
    {
        assert(index < vertices.size());
        auto &ref = vertices[index];
        return std::get<Vertex>(ref);
    }

    template <typename Vertex, typename... Args>
    std::pair<VertexIndexT, Vertex &> add_vertex(Args &&...args)
    {
        const auto index = vertices.size();
        vertices.emplace_back(Vertex { std::forward<Args>(args)... });
        auto &ref = vertex<Vertex>(index);
        static_cast<EventHandler*>(this)->on_vertex_added(ref);
        return { index, ref };
    }

    template <typename VertexFrom, typename VertexTo>
    void add_edge(const VertexIndexT from, const VertexIndexT to)
    {
        auto &vf = vertex<VertexFrom>(from);
        auto &vt = vertex<VertexTo>(to);
        edges.emplace(from, to);
        static_cast<EventHandler*>(this)->on_edge_added(from, vf, to, vt);
    }
};
}
