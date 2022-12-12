#pragma once

namespace usagi
{
struct /*[[deprecated]]*/ TaskProfile
{
    /*
     * Performance Model
     */

    // Computational cost on single reference processor.
    double base_comp_cost = 0;
    // Fraction of sequential part of the task. Used in Amdalh's speed model.
    // Might be an oversimplified measure.
    double frac_seq = 0.1;

    // Temporary fake performance model
    enum PerfType
    {
        COMPUTE_BOUND,
        MEMORY_BOUND,
    } type;

    /*
     * Performance Counters
     */
};

inline auto eval_vertex_weight(const TaskProfile &v)
{
    return v.base_comp_cost;
}
}
