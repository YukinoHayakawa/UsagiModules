#pragma once

#include <tuple>
#include <array>
#include <type_traits>

#include <Usagi/Game/System.hpp>
#include <Usagi/Game/Entity/Component.hpp>
#include <Usagi/Game/detail/ComponentAccessSystemAttribute.hpp>
#include <Usagi/Library/Meta/CompileTimeError.hpp>

namespace usagi
{
// Credit for struct Index:
// https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type
template <class T, class Tuple>
struct Index;

template <class T, class... Types>
struct Index<T, std::tuple<T, Types...>>
{
    static const std::size_t value = 0;
};

template <class T, class U, class... Types>
struct Index<T, std::tuple<U, Types...>>
{
    static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
};

template <class T, class Tuple>
constexpr std::size_t INDEX = Index<T, Tuple>::value;

// Credit of has_type:
// https://stackoverflow.com/a/41171291/2150446

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...>
{
};

template <typename T, typename Tuple>
constexpr bool has_type_v = has_type<T, Tuple>::value;

static_assert(has_type_v<int, std::tuple<int>>);
static_assert(!has_type_v<float, std::tuple<int>>);
static_assert(has_type_v<float, std::tuple<int, float>>);

template <int Size>
struct Stack
{
    std::array<int, Size> stack;
    std::size_t pos = 0;

    constexpr void push(const int value)
    {
        stack[pos++] = value;
    }

    constexpr int pop()
    {
        return stack[--pos];
    }
};


template <int Size>
struct Queue
{
    std::array<int, Size> queue;
    std::size_t begin = 0, end = 0;

    constexpr void push(const int value)
    {
        queue[end++] = value;
    }

    constexpr int pop()
    {
        return queue[begin++];
    }

    constexpr bool empty() const
    {
        return begin == end;
    }

    constexpr int back() const
    {
        return queue[end - 1];
    }
};

enum class ErrorCode
{
    SUCCEED,
    CDG_MULTIPLE_WRITE_PATHS,
};

using TaskGraphError = CompileTimeError<
    ErrorCode,
    int, // vertex index
    ErrorCode::SUCCEED
>;

using TaskGraphErrorCode = TaskGraphError::ErrorCode;

template <ErrorCode Code, int Vertex>
using TaskGraphErrorCodeCheck = TaskGraphError::CheckErrorCode<Code, Vertex>;

template <int Size>
struct AdjacencyMatrixGraph
{
    bool matrix[Size][Size] { };
    int in_degree[Size] { };
    int out_degree[Size] { };

    ComponentAccess system_traits[Size] { };

    constexpr AdjacencyMatrixGraph()
    {
    }

    constexpr void add_edge(const int from, const int to)
    {
        matrix[from][to] = 1;
        ++in_degree[to];
        ++out_degree[from];
    }

    constexpr void remove_edge(const int from, const int to)
    {
        matrix[from][to] = 0;
        --in_degree[to];
        --out_degree[from];
    }

    constexpr bool is_root(const int v) const
    {
        return in_degree[v] == 0;
    }

    constexpr bool has_edge(const int from, const int to) const
    {
        return matrix[from][to] != 0;
    }

    constexpr bool operator==(const AdjacencyMatrixGraph &rhs) const
    {
        for(auto i = 0; i < Size; ++i)
            for(auto j = 0; j < Size; ++j)
                if(matrix[i][j] != rhs.matrix[i][j])
                    return false;

        for(auto i = 0; i < Size; ++i)
            if(system_traits[i] != rhs.system_traits[i])
                return false;

        return true;
    }

    constexpr bool operator!=(const AdjacencyMatrixGraph &rhs) const
    {
        return !(*this == rhs);
    }

    using Flag = std::array<bool, Size>;

    // Ref: https://www.geeksforgeeks.org/detect-cycle-in-a-graph/
    constexpr bool is_cyclic_helper(int v, Flag &visited, Flag &visiting) const
    {
        if(!visited[v])
        {
            visited[v] = true;
            // if this vertex is not visited, it cannot be on the recursive
            // stack
            visiting[v] = true;

            // visit children of v
            for(auto i = 0; i < Size; ++i)
            {
                // not connected
                if(matrix[v][i] == false)
                    continue;
                // new child, dive in
                if(!visited[i])
                {
                    if(is_cyclic_helper(i, visited, visiting))
                        return true;
                }
                // getting back to one of the ancestor -> a cycle was found
                else if(visiting[i])
                    return true;
            }

            visiting[v] = false;
        }
        return false;
    }

    constexpr bool is_cyclic() const
    {
        std::array<bool, Size> visited { };
        std::array<bool, Size> visiting { };

        for(auto i = 0; i < Size; ++i)
        {
            if(is_cyclic_helper(i, visited, visiting))
                return true;
        }
        return false;
    }

    constexpr void transitive_reduce_helper(const int v, const int child)
    {
        // visit indirect descendents of v through child
        for(auto i = 0; i < Size; ++i)
        {
            if(matrix[child][i] == false)
                continue;

            // otherwise, there is an indirect path from v to i, remove the
            // direct path from v to i.
            remove_edge(v, i);

            transitive_reduce_helper(v, i);
        }
    }

    // Ref: https://cs.stackexchange.com/a/29133
    constexpr void transitive_reduce()
    {
        // perform DFS on all vertices, assuming the graph is a DAG
        for(auto v = 0; v < Size; ++v)
        {
            // visit children of v
            for(auto i = 0; i < Size; ++i)
            {
                // not child of v
                if(matrix[v][i] == false)
                    continue;

                transitive_reduce_helper(v, i);
            }
        }
    }
};

template <int Size>
constexpr bool cdg_no_write_systems(const AdjacencyMatrixGraph<Size> &cdg)
{
    for(auto i = 0; i < Size; ++i)
    {
        if(cdg.system_traits[i] == ComponentAccess::WRITE)
            return false;
    }
    return true;
}

template <int Size>
constexpr TaskGraphErrorCode cdg_validate_helper1(
    const AdjacencyMatrixGraph<Size> &cdg,
    const int v,
    Queue<Size> &cwp,
    bool &backtracking)
{
    if(cdg.system_traits[v] == ComponentAccess::WRITE)
    {
        if(backtracking)
        {
            return TaskGraphErrorCode(
                ErrorCode::CDG_MULTIPLE_WRITE_PATHS,
                v
            );
        }
        cwp.push(v);
    }

    // visit children of v
    for(auto i = 0; i < Size; ++i)
    {
        // not child of v
        if(cdg.matrix[v][i] == false)
            continue;

        const auto result = cdg_validate_helper1(
            cdg, i, cwp, backtracking
        );
        if(!result) return result;
    }

    // We have spotted a path containing write systems and we are
    // tracing back along the path. This only finishes the visiting
    // of the current vertex and its descendents. If any other write
    // system was found after backtracking, we have detected another
    // write path and report it as an error.
    if(!cwp.empty() && cwp.back() == v)
    {
        backtracking = true;
    }

    return TaskGraphErrorCode(ErrorCode::SUCCEED);
}

template <int Size>
constexpr TaskGraphErrorCode cdg_validate(const AdjacencyMatrixGraph<Size> &cdg)
{
    // The CDG is validated as per the descriptions in:
    // https://yuki.moe/blog/index.php/2020/03/30/concurrent-entity-access/

    // 1. If all Systems are Read Systems, no execution dependency is needed
    //   since all of them will see the same data.

    if(cdg_no_write_systems(cdg))
        return TaskGraphErrorCode(ErrorCode::SUCCEED);

    // 2. If there exists at least one Write System:
    //
    //     - There must exist one and only one acyclic path that contains
    //       all Write Systems, call this path the Critical Write Path (CWP)
    //       for C.

    // Perform DFS on all vertices with in-degree of 0 in the DAG. Track
    // the first path where a write system was found which forms the CWP.
    Queue<Size> cwp;
    auto backtracking = false;
    for(auto i = 0; i < Size; ++i)
    {
        if(cdg.in_degree[i] == 0)
        {
            const auto result = cdg_validate_helper1(
                cdg, i, cwp, backtracking
            );
            if(!result) return result;
        }
    }

    //     - A Read System must:
    //
    //         - directly depends on the Begin Task without any
    //           Write System in the middle, or
    //
    //         - directly depends on a Write System, or
    //
    //         - directly depends on a Read System satisfying any of this or
    //           the previous two requirements.
    //
    // - For any two adjacent Write System A and B in the traversal order of
    //   the Critical Write Path skipping any Read Systems on the path, if B
    //   indirectly depends on A, define all the Read Systems that directly
    //   or indirectly depend on A but not B the Dependent Read Group (DRG)
    //   of A. Then, for each System in this group, there must be at least
    //   one path from B to A containing this System.

    for(auto i = 0; i < Size; ++i)
    {
        if(cdg.system_traits[i] != ComponentAccess::READ)
            continue;


    }

    return TaskGraphErrorCode(ErrorCode::SUCCEED);
}

template <System... Sys>
class TaskGraph : AdjacencyMatrixGraph<sizeof...(Sys)>
{
public:
    constexpr static std::size_t NUM_SYSTEMS = sizeof...(Sys);
    using SystemList = std::tuple<Sys...>;
    using Graph = AdjacencyMatrixGraph<NUM_SYSTEMS>;

    template <System From, System To>
    constexpr void precede()
    {
        static_assert(has_type_v<From, SystemList>);
        static_assert(has_type_v<To, SystemList>);

        this->add_edge(
            INDEX<From, SystemList>,
            INDEX<To, SystemList>
        );
    }

    template <Component C>
    constexpr Graph component_dependency_graph() const
    {
        Graph cdg;
        // visit all possible vertices
        cdg_helper_row<C>(cdg, std::make_index_sequence<NUM_SYSTEMS>());
        return cdg;
    }


    constexpr Graph reduced_task_graph() const
    {

    }

private:
    template <Component C, std::size_t From, std::size_t To>
    constexpr void cdg_helper_edge(Graph &cdg) const
    {
        using SystemFrom = std::tuple_element_t<From, SystemList>;
        using SystemTo = std::tuple_element_t<To, SystemList>;

        constexpr bool sys_from = SystemCanAccessComponent<SystemFrom, C>;
        constexpr bool sys_to = SystemCanAccessComponent<SystemTo, C>;

        // If both systems access the same component, the edge is preserved
        if(sys_from && sys_to && this->has_edge(From, To))
        {
            cdg.add_edge(From, To);
        }
    }

    template <Component C, std::size_t I, std::size_t... CIs>
    constexpr void cdg_helper_children(
        Graph &cdg,
        std::index_sequence<CIs...>) const
    {
        // visit each child of I
        (..., cdg_helper_edge<C, I, CIs>(cdg));
    }

    template <Component C, std::size_t I>
    constexpr void cdg_helper_vertex(Graph &cdg) const
    {
        // visit children of system I
        cdg.system_traits[I] = SystemHighestComponentAccess<
            std::tuple_element_t<I, SystemList>,
            C
        >;
        cdg_helper_children<C, I>(cdg, std::make_index_sequence<NUM_SYSTEMS>());
    }

    template <Component C, std::size_t... Is>
    constexpr void cdg_helper_row(Graph &cdg, std::index_sequence<Is...>) const
    {
        // visit every system
        (..., cdg_helper_vertex<C, Is>(cdg));
    }
};
}
