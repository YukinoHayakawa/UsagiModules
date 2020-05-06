#pragma once

#include <map>
#include <vector>
#include <deque>
#include <set>
#include <iostream>

#include <fmt/printf.h>

namespace usagi
{
struct Graph
{
    using KeyT = int;

    // Edges point from the key to the values
    std::map<KeyT, std::vector<KeyT>> adjacent_list;

    void add_edge(const int from, const int to)
    {
        adjacent_list[from].push_back(to);
    }
};

// ref: https://www.geeksforgeeks.org/topological-sorting/
struct TopologicalSort
{
    Graph &graph;
    std::deque<int> result;
    std::set<int> visited;

    explicit TopologicalSort(Graph &graph)
        : graph(graph)
    {
    }

    void helper(const int v)
    {
        visited.insert(v);

        // recurse on all children
        for(auto &&c : graph.adjacent_list[v])
            if(!visited.contains(c))
                helper(c);

        // ensure the current vertex precedes the children
        result.push_front(v);
    }

    void sort(const int start)
    {
        result.clear();
        visited.clear();

        helper(start);

        for(auto &&v : result)
            std::cout << v << " ";
        std::cout << std::endl;
    }
};

/*
struct TopologicalSortMulti
{
    Graph &graph;
    std::map<int, std::deque<int>> results;
    std::deque<int> visiting;
    std::set<int> visited;

    explicit TopologicalSortMulti(Graph &graph)
        : graph(graph)
    {
    }

    void helper(const int v)
    {
        fmt::print("push {}\n", v);
        visiting.push_back(v);

        visited.insert(v);

        // recurse on all children
        for(auto &&c : graph.adjacent_list[v])
        {
            if(!visited.contains(c))
                helper(c);
            results[c].push_front(v);
            fmt::print("insert {} to {}\n", v, c);
        }

        // ensure the current vertex precedes the children

        fmt::print("pop {}\n", v);
        visiting.pop_back();
    }

    void sort(const int start)
    {
        results.clear();
        visited.clear();

        helper(start);

        for(auto &&r : results)
        {
            std::cout << "[" << r.first << "] ";
            for(auto &&v : r.second)
                std::cout << v << " ";
            std::cout << std::endl;
        }
    }
};
*/

}
