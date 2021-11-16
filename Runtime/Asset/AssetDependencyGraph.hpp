#pragma once

#include <cassert>
#include <deque>

#include <Usagi/Library/Graph/AdjacencyList.hpp>

namespace usagi
{
template <typename VertexT>
class AssetDependencyGraph : public graph::AdjacencyList<VertexT>
{
public:
    using GraphT = graph::AdjacencyList<VertexT>;
    using VertexIndexT = typename GraphT::VertexIndexT;

private:
    std::deque<VertexIndexT> mFreedIndices;

    using GraphT::resize;

public:
    VertexIndexT add_vertex()
    {
        VertexIndexT idx;
        if(!mFreedIndices.empty())
        {
            idx = mFreedIndices.back();
            mFreedIndices.pop_back();
        }
        else
        {
            idx = this->mVertices.size();
            this->mVertices.emplace_back();
        }
        auto &v = this->mVertices[idx];
        assert(v.adj.empty());
        return idx;
    }

    void erase_vertex(VertexIndexT v)
    {
        this->mVertices[v] = typename GraphT::VertexInfo();
        mFreedIndices.emplace_back(v);
    }

    void dump_graph()
    {
    }
};
}
