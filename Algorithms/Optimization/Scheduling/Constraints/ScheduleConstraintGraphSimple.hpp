#pragma once

#include <ranges>
#include <algorithm>
#include <queue>
#include <Usagi/Modules/Common/Logging/Logging.hpp>

#include "ScheduleConstraintGraph.hpp"
#include "Nodes/ScheduleNodeProcessorReady.hpp"
#include "Nodes/ScheduleNodeTaskPrecedence.hpp"

namespace usagi
{
struct ScheduleNodeRoot
{
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
    std::priority_queue<int> task_queue;

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

    // assigning a task to a processor causes it to be executed
    void on_edge_added(
        VertexIndexT from_idx,
        ScheduleNodeProcessorReady &from_ready,
        VertexIndexT to_idx,
        ScheduleNodeExec &to_exec)
    {
        // previous ready time is bounded by precedence constraints

        to_exec.ready_time = std::max(from_ready.time, to_exec.ready_time);
        to_exec.finish_time = to_exec.ready_time + to_exec.exec_time;
        // to_exec.exec_time = 123;

        // propagate update upcoming barriers & tasks
        // note: exec nodes are connected with barrier nodes at the beginning
        // of the whole process

        // todo: nodes might be repeatedly visited?
        auto visit_out_edges = [&]<typename FromVertex, typename ToVertex>(
            VertexIndexT from_idx_,
            FromVertex &from_v_,
            VertexIndexT to_idx_,
            ToVertex &&to_v_) {
            propagate_update_barriers<FromVertex>(from_idx_, from_v_, to_idx_, to_v_);
        };

        visit_outgoing_edges<ScheduleNodeExec>(to_idx, visit_out_edges);

        // update proc ready time

        auto [next_idx, next_ready] = add_vertex<ScheduleNodeProcessorReady>();
        next_ready.proc_id = from_ready.proc_id;
        next_ready.time = to_exec.finish_time;

        add_edge<ScheduleNodeExec, ScheduleNodeProcessorReady>(to_idx, next_idx);
    }

    template <typename FromVertex, typename ToVertex>
    void propagate_update_barriers(
        VertexIndexT from_idx,
        FromVertex &from_v,
        VertexIndexT to_idx,
        ToVertex &&to_v)
    {
        auto recursive_self = [&]<typename FromVertex_, typename ToVertex_>(
            VertexIndexT from_idx_,
            FromVertex_ &from_v_,
            VertexIndexT to_idx_,
            ToVertex_ &&to_v_) {
            propagate_update_barriers<FromVertex_>(from_idx_, from_v_, to_idx_, to_v_);
        };

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
                        recursive_self
                    );
                }
                // otherwise stops here
            }
        }
        // if a barrier->exec edge is visited, the exec node should be
        // scheduled and the graph traversal should be terminated.
        else if constexpr(std::is_same_v<To, ScheduleNodeExec>)
        {
            if constexpr(std::is_same_v<FromVertex, ScheduleNodeBarrier>)
            {
                to_v.ready_time = from_v.finish_time;
                LOG(error, "task queued {}[{}]", to_v.task_id, to_v.subtask_id);
            }
        }
        // ignore other nodes
    }

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
};
}
