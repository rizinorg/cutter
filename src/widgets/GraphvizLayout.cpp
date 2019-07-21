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
    width = height = 10;
    GVC_t* gvc = gvContext();
    Agraph_t* g = agopen(const_cast<char*>("G"), Agdirected, nullptr); //Leftovers from old C library, graphviz shouldn't modify file name

    std::unordered_map<ut64, Agnode_t*> nodes;
    for (const auto& block : blocks)
    {
        nodes[block.first] = agnode(g, nullptr, TRUE);
    }

    std::vector<std::string> strc;
    strc.reserve(2 * blocks.size());
    std::map<std::pair<ut64, ut64>, Agedge_t*> edges;

    agsafeset(g, (char*)"splines", (char*)"ortho", "");
    agsafeset(g, (char*)"rankdir", (char*)"BT", "");
    const float dpi = 72.0;
    agsafeset(g, (char*)"dpi", (char*)"72", "");
    for (const auto& blockIt : blocks)
    {
        auto u = nodes[blockIt.first];
        auto& block = blockIt.second;

        for (auto & edge : block.edges)
        {
            auto v = nodes.find(edge.target);
            if (v == nodes.end()) {
                continue;
            }
            auto e = agedge(g,u,v->second,nullptr,TRUE);
            edges[{blockIt.first, edge.target}] = e;
        }
        std::string str = std::to_string(1.0 * block.width / dpi);
        std::replace(str.begin(), str.end(), ',', '.');
        strc.push_back(str);
        agsafeset(u, (char*)"width", (char*)strc.back().c_str(), "");
        str = std::to_string(1.0 * block.height / dpi);
        std::replace(str.begin(), str.end(), ',', '.');
        strc.push_back(str);
        agsafeset(u, (char*)"height", (char*)strc.back().c_str(), "");

        agsafeset(u, (char*)"shape", (char*)"box", "");
        agsafeset(u, (char*)"fixedsize", (char*)"true", "");
    }

    gvLayout(gvc, g, "dot");
    gvRender (gvc, g, "dot", nullptr);

    for (auto& blockIt : blocks)
    {
        auto& block = blockIt.second;
        auto u = nodes[blockIt.first];

        auto pos = ND_coord(u);

        auto w = ND_width(u) * dpi;
        auto h = ND_height(u) * dpi;
        block.x = pos.x  - w / 2.0;
        block.y = pos.y  - h / 2.0;
        width = std::max(width, block.x + block.width);
        height = std::max(height, block.y + block.height);

        for (auto & edge : block.edges)
        {
            auto it = edges.find({blockIt.first, edge.target});
            if (it != edges.end()) {
                auto e = it->second;
                if (auto spl = ED_spl(e)) {
                    for (int i=0; i<1 && i < spl->size; i++) {
                        auto bz = spl->list[i];
                        edge.polyline.reserve(bz.size + 1);
                        for (int j=0; j<bz.size; j++) {
                            edge.polyline.push_back(QPointF(bz.list[j].x, bz.list[j].y));
                        }
                        QPointF last(0, 0);
                        if (!edge.polyline.empty()) {
                            last = edge.polyline.back();
                        }
                        if (bz.eflag) {
                            QPointF tip = QPointF(bz.ep.x, bz.ep.y);
                            edge.polyline.push_back(tip);
                            QPointF direction = tip - last;
                            if (abs(direction.x()) > abs(direction.y())) {
                                if (direction.x() > 0) {
                                    edge.arrow = GraphEdge::Right;
                                } else {
                                    edge.arrow = GraphEdge::Left;
                                }
                            } else {
                                if (direction.y() > 0) {
                                    edge.arrow = GraphEdge::Down;
                                } else {
                                    edge.arrow = GraphEdge::Up;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    gvFreeLayout(gvc, g);
    for (auto v : nodes)
    {
        agdelnode(g, v.second);
    }

    agclose(g);
    gvFreeContext(gvc);
}
