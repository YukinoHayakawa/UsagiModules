#pragma once

#include <type_traits>
#include <cassert>

#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecutionBarrier.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Execution/ScheduleNodeExecuteTask.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Processors/ScheduleNodeProcessorReady.hpp>
#include <Usagi/Modules/Algorithms/Optimization/Scheduling/Aspects/Root/ScheduleNodeRoot.hpp>

namespace usagi
{
template <
    typename Graph,
    typename VertexIndexT = typename Graph::VertexIndexT,
    typename TaskIndexT = typename Graph::TaskIndexT
>
struct ScheduleEventHandlerPrecedenceConstraints
{
    // Check the waiting number of barriers and enqueue ready tasks caused
    // by satisfied barriers.
    template <typename FromVertex, typename ToVertex>
    void propagate_update_barriers(
        VertexIndexT from_idx,
        FromVertex &from_v,
        VertexIndexT to_idx,
        ToVertex &&to_v)
    {
        using To = std::decay_t<ToVertex>;
        using Barrier = ScheduleNodeExecutionBarrier<TaskIndexT>;
        using Exec = ScheduleNodeExecuteTask<TaskIndexT>;

        const auto graph = [this] { return static_cast<Graph *>(this); };

        // propagate update upcoming barriers.
        
        // update descendent barrier waiting state when an immediate ancestor
        // is finished execution. the ancestor would either be a finished
        // task or a fulfilled barrier. note that a exec node must not
        // directly link to another exec node. a barrier must be inserted
        // in-between to handle the precedence constraint.
        // task execution nodes will not be visited multiple times because
        // a barrier will only be fulfilled once.
        if constexpr(std::is_same_v<To, Barrier>)
        {
            if constexpr(
                std::is_same_v<FromVertex, Barrier> ||
                std::is_same_v<FromVertex, Exec>)
            {
                // because there is an incoming edge to the barrier, it must
                // be waiting.
                assert(to_v.num_waiting_inputs > 0);
                --to_v.num_waiting_inputs;
                // potentially delay the ready time
                to_v.update_ready_time(from_v.finish_time);
                // barrier satisfied, recursively update upcoming nodes.
                if(to_v.num_waiting_inputs == 0)
                {
                    to_v.finish_time = to_v.ready_time;
                    // propagate ready state
                    graph()->template visit_outgoing_edges<
                        Barrier
                    >(to_idx, propagate_update_barriers_func());
                }
                // otherwise stops here
            }
        }
        // if a barrier->exec edge is visited, the exec node should be
        // enqueued and the graph traversal on the current branch should be
        // terminated.
        else if constexpr(std::is_same_v<To, Exec>)
        {
            if constexpr(std::is_same_v<FromVertex, Barrier>)
            {
                // note: do not directly take the finish time of the barrier
                // as the ready time of the task as it may also wait for
                // communications, etc.
                to_v.update_ready_time(from_v.finish_time);
                graph()->enqueue_task(to_idx);
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
            propagate_update_barriers<FromVertex>(
                from_idx_, from_v_, to_idx_, to_v_);
        };
    }

    // assigning a task to a processor causes it to be executed
    void on_edge_added(
        VertexIndexT from_idx,
        ScheduleNodeProcessorReady &from_ready,
        VertexIndexT to_idx,
        ScheduleNodeExecuteTask<TaskIndexT> &to_exec)
    {
        const auto graph = [this] { return static_cast<Graph *>(this); };

        // occupy the processor
        from_ready.occupy();

        // previous ready time is bounded by precedence constraints, etc
        to_exec.update_ready_time(from_ready.ready_time);
        to_exec.derive_finish_time();

        // create another proc ready node so it can be used to execute other
        // tasks later.
        auto [next_idx, next_ready] = 
            graph()->template add_vertex<ScheduleNodeProcessorReady>();
        next_ready.processor_index = from_ready.processor_index;
        next_ready.ready_time = to_exec.finish_time;

        // link the current task being executed to the next proc ready state
        // to form the timeline path.
        graph()->template add_edge<
            ScheduleNodeExecuteTask<TaskIndexT>,
            ScheduleNodeProcessorReady
        >(to_idx, next_idx);

        // propagate update upcoming barriers & tasks
        // note: exec nodes are connected with barrier nodes at the beginning
        // of the whole process
        graph()->template visit_outgoing_edges<
            ScheduleNodeExecuteTask<TaskIndexT>
        >(to_idx, propagate_update_barriers_func());
    }

    // link the root node to the beginning barrier of the source task
    // to boot the scheduling process.
    void on_edge_added(
        const VertexIndexT from_idx,
        ScheduleNodeRoot &from_root,
        const VertexIndexT to_idx,
        ScheduleNodeExecutionBarrier<TaskIndexT> &to_barrier)
    {
        const auto graph = [this] { return static_cast<Graph *>(this); };

        assert(to_barrier.num_waiting_inputs == 0);
        graph()->template visit_outgoing_edges<
            ScheduleNodeExecutionBarrier<TaskIndexT>
        >(to_idx, propagate_update_barriers_func());
    }

    void on_edge_added(
        const VertexIndexT from_idx,
        ScheduleNodeExecuteTask<TaskIndexT> &from_exec,
        const VertexIndexT to_idx,
        ScheduleNodeExecutionBarrier<TaskIndexT> &to_barrier)
    {
        ++to_barrier.num_waiting_inputs;
    }

    void on_edge_added(
        const VertexIndexT from_idx,
        ScheduleNodeExecutionBarrier<TaskIndexT> &from_barrier,
        const VertexIndexT to_idx,
        ScheduleNodeExecutionBarrier<TaskIndexT> &to_barrier)
    {
        ++to_barrier.num_waiting_inputs;
    }
};
}
