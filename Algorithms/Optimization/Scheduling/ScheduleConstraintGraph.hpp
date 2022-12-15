#pragma once

#include <map>
#include <variant>
#include <deque>
#include <cassert>

#include <range/v3/range.hpp>
#include <range/v3/view.hpp>
#include <Usagi/Runtime/ErrorHandling.hpp>

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
    typename VertexIndex,
    typename EventHandler,
    typename... EnabledNodes // put in std::variant?
>
struct ScheduleConstraintGraph
{
    using VertexT = std::variant<EnabledNodes...>;
    using VertexIndexT = VertexIndex;

    // deque is used to keep references valid
    std::deque<VertexT> vertices;
    std::multimap<VertexIndexT, VertexIndexT> edges;

    template <typename Vertex>
    Vertex & vertex(VertexIndexT index)
    {
        assert(index < vertices.size());
        auto &ref = vertices[index];
        // will throw if vertex type doesn't match
        return std::get<Vertex>(ref);
    }

    template <typename Vertex, typename... Args>
    std::pair<VertexIndexT, Vertex &> add_vertex(Args &&...args)
    {
        USAGI_ASSERT_THROW(
            vertices.size() < static_cast<std::size_t>(
                std::numeric_limits<VertexIndexT>::max()),
            std::overflow_error("vertex index exceeds the maximum value"
                " of the designated index type."));

        const auto index = static_cast<VertexIndexT>(vertices.size());
        vertices.emplace_back(Vertex { std::forward<Args>(args)... });
        auto &ref = vertex<Vertex>(index);
        static_cast<EventHandler*>(this)->on_vertex_added(index, ref);

        return { index, ref };
    }

    template <typename VertexFrom, typename VertexTo>
    void add_edge(const VertexIndexT from, const VertexIndexT to)
    {
        // validate the existence of vertices and their types
        auto &vf = vertex<VertexFrom>(from);
        auto &vt = vertex<VertexTo>(to);
        edges.emplace(from, to);
        static_cast<EventHandler*>(this)->on_edge_added(from, vf, to, vt);
    }

    template <typename FromVertex>
    void visit_outgoing_edges(const VertexIndexT from_idx, auto &&visitor)
    {
        const auto [begin, end] = edges.equal_range(from_idx);
        auto &from_v = vertex<FromVertex>(from_idx);
        for(auto it = begin; it != end; ++it)
        {
            const auto invoke_with_idx = [&]<typename Vertex>(Vertex &&to_v)
            {
                static_assert(std::is_lvalue_reference_v<Vertex &&>);
                // also pass the index of the dest vertex to the visitor
                visitor.template operator()<FromVertex>(
                    from_idx,
                    from_v,
                    it->second,
                    std::forward<Vertex>(to_v)
                );
            };
            auto &to_v = vertices[it->second];
            std::visit(invoke_with_idx, to_v);
        }
    }

    auto indexed_vertices()
    {
        return // zip vertices with their indices
            ranges::views::zip(
                ranges::views::iota(0ull, vertices.size()),
                ranges::views::all(vertices)
            );
    }

    template <typename FilterFunc>
    auto filtered_vertices(FilterFunc &&filter)
    {
        return indexed_vertices() | 
            ranges::views::filter(std::forward<FilterFunc>(filter));
    }

    template <typename Vertex>
    static auto func_get_vertices_as()
    {
        return ranges::views::transform(
            [](auto &&idx_v_pair) {
                return std::tuple<VertexIndexT, Vertex &> {
                    std::get<0>(idx_v_pair),
                    std::get<Vertex>(std::get<1>(idx_v_pair))
                };
            }
        );
    }
};
}
