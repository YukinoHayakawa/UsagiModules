#pragma once

#include <tuple>
#include <array>

#include <Usagi/Game/System.hpp>

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

    constexpr AdjacencyMatrixGraph()
    {
    }

    constexpr void add_edge(const int from, const int to)
    {
        matrix[from][to] = 1;
    }

    using Flag = std::array<bool, Size>;

    // Ref: https://www.geeksforgeeks.org/detect-cycle-in-a-graph/
    constexpr bool is_cyclic_helper(int v, Flag &visited, Flag &visiting)
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
                else if(visiting[i]) { return true; }
            }

            visiting[v] = false;
        }
        return false;
    }

    constexpr bool is_cyclic()
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
};

template <System... Sys>
using SystemList = std::tuple<Sys...>;

template <System From, System To, typename SystemList>
constexpr void precede(
    AdjacencyMatrixGraph<std::tuple_size_v<SystemList>> &task_graph)
{
    task_graph.add_edge(INDEX<From, SystemList>, INDEX<To, SystemList>);
}

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
