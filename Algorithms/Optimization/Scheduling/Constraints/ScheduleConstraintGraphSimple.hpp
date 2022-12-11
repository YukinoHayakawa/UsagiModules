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
#include "Nodes/ScheduleNodeTaskPrecedence.hpp"

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
        ScheduleNodeProcessorReady,
        ScheduleNodeExec,
        ScheduleNodeBarrier
    >
{
    std::priority_queue<VertexIndexT> task_queue;

    template <typename FromVertex>
    void visit_outgoing_edges(const VertexIndexT from_idx, auto &&visitor)
    {
        const auto [begin, end] = edges.equal_range(from_idx);
        auto &from_v = vertex<FromVertex>(from_idx);
        for(auto it = begin; it != end; ++it)
        {
            auto &vertex_ref = vertices[it->second];
            std::visit(
                [&]<typename Vertex>(Vertex &&to_v) {
                    // also pass the index of the dest vertex to the visitor
                    visitor.template operator()<FromVertex>(
                        from_idx,
                        from_v,
                        it->second,
                        std::forward<Vertex>(to_v)
                    );
                },
                vertex_ref
            );
        }
    }

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

    template <typename FromVertex, typename ToVertex>
    void propagate_update_barriers(
        VertexIndexT from_idx,
        FromVertex &from_v,
        VertexIndexT to_idx,
        ToVertex &&to_v)
    {
        // auto recursive_self = [&]<typename FromVertex_, typename ToVertex_>(
        //     VertexIndexT from_idx_,
        //     FromVertex_ &from_v_,
        //     VertexIndexT to_idx_,
        //     ToVertex_ &&to_v_) {
        //     propagate_update_barriers<FromVertex_>(from_idx_, from_v_, to_idx_, to_v_);
        // };

        using To = std::decay_t<ToVertex>;

        // propagate update upcoming barriers. note that a exec node must not
        // directly link to another exec node. a barrier must be inserted
        // in-between to handle the precedence constraint.
        if constexpr(std::is_same_v<To, ScheduleNodeBarrier>)
        {
            if constexpr(
                std::is_same_v<FromVertex, ScheduleNodeExec> ||
                std::is_same_v<FromVertex, ScheduleNodeBarrier>)
            {
                // because there is an incoming edge to the barrier, it must
                // be waiting.
                assert(to_v.num_waiting_input > 0);
                --to_v.num_waiting_input;
                to_v.ready_time = std::max(to_v.ready_time, from_v.finish_time);
                // barrier satisfied, recursively update upcoming nodes.
                if(to_v.num_waiting_input == 0)
                {
                    to_v.finish_time = to_v.ready_time;
                    visit_outgoing_edges<ScheduleNodeBarrier>(
                        to_idx,
                        propagate_update_barriers_func()
                    );
                }
                // otherwise stops here
            }
            // push the initial task into the queue
            // else if constexpr(std::is_same_v<FromVertex, ScheduleNodeRoot>)
            // {
            //     assert(to_v.num_waiting_input == 0);
            //     to_v.finish_time = to_v.ready_time = from_v.ready_time;
            //     visit_outgoing_edges<ScheduleNodeBarrier>(
            //         to_idx,
            //         recursive_self
            //     );
            // }
        }
        // if a barrier->exec edge is visited, the exec node should be
        // scheduled and the graph traversal should be terminated.
        else if constexpr(std::is_same_v<To, ScheduleNodeExec>)
        {
            if constexpr(std::is_same_v<FromVertex, ScheduleNodeBarrier>)
            {
                to_v.ready_time = from_v.finish_time;
                LOG(error, "task queued {}[{}]", to_v.task_id, to_v.subtask_id);
                task_queue.push(to_idx);
            }
        }
        // ignore other nodes
    }

    auto propagate_update_barriers_func()
    {
        return [&]<typename FromVertex, typename ToVertex>(
            VertexIndexT from_idx_,
            FromVertex &from_v_,
            VertexIndexT to_idx_,
            ToVertex &&to_v_) {
            propagate_update_barriers<FromVertex>(from_idx_, from_v_, to_idx_, to_v_);
        };
    }

    // assigning a task to a processor causes it to be executed
    void on_edge_added(
        VertexIndexT from_idx,
        ScheduleNodeProcessorReady &from_ready,
        VertexIndexT to_idx,
        ScheduleNodeExec &to_exec)
    {
        // occupy the processor

        assert(from_ready.occupied == false);
        from_ready.occupied = true;

        // previous ready time is bounded by precedence constraints

        to_exec.ready_time = std::max(from_ready.ready_time, to_exec.ready_time);
        to_exec.finish_time = to_exec.ready_time + to_exec.exec_time;
        // to_exec.exec_time = 123;

        // update proc ready time

        auto [next_idx, next_ready] = add_vertex<ScheduleNodeProcessorReady>();
        next_ready.proc_id = from_ready.proc_id;
        next_ready.ready_time = to_exec.finish_time;

        add_edge<ScheduleNodeExec, ScheduleNodeProcessorReady>(to_idx, next_idx);

        // propagate update upcoming barriers & tasks
        // note: exec nodes are connected with barrier nodes at the beginning
        // of the whole process

        // todo: nodes might be repeatedly visited?
        // auto visit_out_edges = [&]<typename FromVertex, typename ToVertex>(
        //     VertexIndexT from_idx_,
        //     FromVertex &&from_v_,
        //     VertexIndexT to_idx_,
        //     ToVertex &&to_v_) {
        //     propagate_update_barriers<FromVertex>(from_idx_, from_v_, to_idx_, to_v_);
        // };

        visit_outgoing_edges<ScheduleNodeExec>(to_idx, propagate_update_barriers_func());
    }

    // push source tasks

    void on_edge_added(
        const VertexIndexT from_idx,
        ScheduleNodeRoot &from_root,
        const VertexIndexT to_idx,
        ScheduleNodeBarrier &to_barrier)
    {
        // auto visit_out_edges = [&]<typename FromVertex, typename ToVertex>(
        //     VertexIndexT from_idx_,
        //     FromVertex &from_v_,
        //     VertexIndexT to_idx_,
        //     ToVertex &&to_v_) {
        //     propagate_update_barriers<FromVertex>(from_idx_, from_v_, to_idx_, to_v_);
        // };

        assert(to_barrier.num_waiting_input == 0);
        visit_outgoing_edges<ScheduleNodeBarrier>(to_idx, propagate_update_barriers_func());
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
