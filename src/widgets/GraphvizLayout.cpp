#include "GraphvizLayout.h"

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <cassert>
#include <sstream>
#include <iomanip>

#include <gvc.h>

GraphvizLayout::GraphvizLayout()
    : GraphLayout({})
{
}

void GraphvizLayout::CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry,
                                     int &width, int &height) const
{
    //https://gitlab.com/graphviz/graphviz/issues/1441
#define STR(v) const_cast<char*>(v)

    width = height = 10;
    GVC_t *gvc = gvContext();
    Agraph_t *g = agopen(STR("G"), Agdirected, nullptr);

    std::unordered_map<ut64, Agnode_t *> nodes;
    for (const auto &block : blocks) {
        nodes[block.first] = agnode(g, nullptr, TRUE);
    }

    std::vector<std::string> strc;
    strc.reserve(2 * blocks.size());
    std::map<std::pair<ut64, ut64>, Agedge_t *> edges;

    agsafeset(g, STR("splines"), STR("ortho"), STR(""));
    agsafeset(g, STR("rankdir"), STR("BT"), STR(""));
    // graphviz has builtin 72 dpi setting for input that differs from output
    // it's easier to use 72 everywhere
    const double dpi = 72.0;
    agsafeset(g, STR("dpi"), STR("72"), STR(""));

    auto widhAttr = agattr(g, AGNODE, STR("width"), STR("1"));
    auto heightAattr = agattr(g, AGNODE, STR("height"), STR("1"));
    agattr(g, AGNODE, STR("shape"), STR("box"));
    agattr(g, AGNODE, STR("fixedsize"), STR("true"));


    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    auto setFloatingPointAttr = [&stream](void *obj, Agsym_t *sym, double value) {
        stream.str({});
        stream << std::fixed << std::setw(4) << value;
        auto str = stream.str();
        agxset(obj, sym, STR(str.c_str()));
    };

    for (const auto &blockIt : blocks) {
        auto u = nodes[blockIt.first];
        auto &block = blockIt.second;

        for (auto &edge : block.edges) {
            auto v = nodes.find(edge.target);
            if (v == nodes.end()) {
                continue;
            }
            auto e = agedge(g, u, v->second, nullptr, TRUE);
            edges[{blockIt.first, edge.target}] = e;
        }
        setFloatingPointAttr(u, widhAttr, block.width / dpi);
        setFloatingPointAttr(u, heightAattr, block.height / dpi);
    }

    gvLayout(gvc, g, "dot");

    for (auto &blockIt : blocks) {
        auto &block = blockIt.second;
        auto u = nodes[blockIt.first];

        auto pos = ND_coord(u);

        auto w = ND_width(u) * dpi;
        auto h = ND_height(u) * dpi;
        block.x = pos.x  - w / 2.0;
        block.y = pos.y  - h / 2.0;
        width = std::max(width, block.x + block.width);
        height = std::max(height, block.y + block.height);

        for (auto &edge : block.edges) {
            auto it = edges.find({blockIt.first, edge.target});
            if (it != edges.end()) {
                auto e = it->second;
                if (auto spl = ED_spl(e)) {
                    for (int i = 0; i < 1 && i < spl->size; i++) {
                        auto bz = spl->list[i];
                        edge.polyline.reserve(bz.size + 1);
                        for (int j = 0; j < bz.size; j++) {
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
    for (auto v : nodes) {
        agdelnode(g, v.second);
    }

    agclose(g);
    gvFreeContext(gvc);
#undef STR
}
