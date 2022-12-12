#pragma once

#include <ranges>
#include <algorithm>
#include <queue>
#include <type_traits>

#include <range/v3/range.hpp>
#include <range/v3/view.hpp>

#include <Usagi/Library/Utilities/Variant.hpp>
#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "ScheduleConstraintGraph.hpp"
#include "Nodes/ScheduleNodeProcessorReady.hpp"

namespace usagi
{
struct ScheduleNodeRoot
{
    float ready_time = 0;
};

struct ScheduleNodeBarrier
{
    float ready_time = -1;
    float finish_time = -1;
    int num_waiting_input = -1;
};

struct ScheduleNodeExec
{
    int task_id;
    int subtask_id;
    float ready_time = -1;
    float exec_time = -1;
    float finish_time = -1;
};

struct ScheduleConstraintGraphSimple
    : ScheduleConstraintGraph<
        ScheduleConstraintGraphSimple,
        ScheduleNodeRoot,
        ScheduleNodeProcessorReady<std::size_t>,
        ScheduleNodeExec,
        ScheduleNodeBarrier
    >
{
    std::priority_queue<VertexIndexT> task_queue;



    void on_vertex_added(auto &v_ref)
    {
        // no-op
    }

    void on_edge_added(
        VertexIndexT from_idx,
        auto &&from_v,
        VertexIndexT to_idx,
        auto &&to_v)
    {
        // ignore other edges
    }


    //
    // void push_ready_barriers()
    // {
    //     visit_vertices(
    //         Overloaded {
    //             [&](VertexIndexT idx, const ScheduleNodeBarrier &barrier) {
    //                 if(barrier.num_waiting_input == 0)
    //
    //             },
    //             [](VertexIndexT idx, auto &&v) { /* no-op*/  }
    //         }
    //     );
    //         /*
    //         VertexIndexT idx, Vertex &&v) {
    //         using V = std::decay_t<Vertex>;
    //         if constexpr(std::is_same_v<V, ScheduleNodeBarrier>)
    //         {
    //             if(v.num_waiting_input)
    //         }
    //         */
    // }


    // todo perf optimization
    auto proc_ready_nodes()
    {
        // using pair = std::tuple<VertexIndexT, VertexT &>;

        return
            ranges::views::zip(
                ranges::views::iota(0ull, vertices.size()),
                ranges::views::all(vertices)) |
            ranges::views::filter([](auto &&p) {
                return std::visit(
                    []<typename Vertex>(Vertex &&vv) -> bool {
                        using V = std::decay_t<Vertex>;
                        if constexpr(std::is_same_v<V, ScheduleNodeProcessorReady>)
                            return vv.occupied == false;
                        return false;
                    }, std::get<1>(p));
            }) |
            ranges::views::transform([](auto &&p) {
                return std::tuple<VertexIndexT, ScheduleNodeProcessorReady &> {
                    std::get<0>(p),
                    std::get<ScheduleNodeProcessorReady>(std::get<1>(p))
                };
            });
    }

    void visit_vertices(auto &&visitor)
    {
        for(VertexIndexT i = 0; auto &&v : vertices)
        {
            std::visit(
                [&]<typename Vertex>(Vertex &&vv) {
                    visitor.template operator()<Vertex>(
                        i,
                        std::forward<Vertex>(vv)
                    );
                },
                v
            );
        }
    }
};
}
