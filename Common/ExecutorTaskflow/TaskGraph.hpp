#pragma once

#include <tuple>
#include <array>
#include <type_traits>

#include <Usagi/Game/System.hpp>
#include <Usagi/Game/Entity/Component.hpp>
#include <Usagi/Game/detail/ComponentAccessSystemAttribute.hpp>

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

template <int Size>
struct AdjacencyMatrixGraph
{
    bool matrix[Size][Size] { };

    ComponentAccess system_traits[Size] { };

    constexpr AdjacencyMatrixGraph()
    {
    }

    constexpr void add_edge(const int from, const int to)
    {
        matrix[from][to] = 1;
    }

    constexpr void remove_edge(const int from, const int to)
    {
        matrix[from][to] = 0;
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


template <int Size>
struct Stack
{
    std::array<int, Size> stack;
    std::size_t pos = 0;

    constexpr void push(const int value)
    {
        stack[pos] = value;
        ++pos;
    }

    constexpr int pop()
    {
        --pos;
        return stack[pos];
    }
};
}
