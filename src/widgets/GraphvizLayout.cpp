#include "GraphvizLayout.h"

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <cassert>

#include <gvc.h>

GraphvizLayout::GraphvizLayout()
    : GraphLayout({})
{
}


void GraphvizLayout::CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry,
                                      int &width, int &height) const
{


    GVC_t* gvc = gvContext();
    Agraph_t* g = agopen("G", Agdirected, nullptr);

    std::unordered_map<ut64, Agnode_t*> nodes;
    for (const auto& block : blocks)
    {
        nodes[block.first] = agnode(g, nullptr, TRUE);
    }


    gvLayout(gvc, g, "dot");

    gvFreeLayout(gvc, g);

    for (auto v : nodes)
    {
        agdelnode(g, v.second);
    }
    for (const auto& block : blocks)
    {
        auto u = nodes[block.first];
        for (auto & edge : block.second.edges)
        {
            auto v = nodes.find(edge.target);
            if (v == nodes.end()) {
                continue;
            }
            auto e = agedge(g,u,v->second,nullptr,TRUE);
        }
    }



    agclose(g);
    gvFreeContext(gvc);
}
