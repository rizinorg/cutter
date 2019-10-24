#include "GraphvizLayout.h"

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <set>

#include <gvc.h>

GraphvizLayout::GraphvizLayout(LineType lineType, Direction direction)
    : GraphLayout({})
    , direction(direction)
    , lineType(lineType)
{
}

static GraphLayout::GraphEdge::ArrowDirection getArrowDirection(QPointF direction,
                                                                bool preferVertical)
{
    if (abs(direction.x()) > abs(direction.y()) * (preferVertical ? 3.0 : 1.0)) {
        if (direction.x() > 0) {
            return GraphLayout::GraphEdge::Right;
        } else {
            return GraphLayout::GraphEdge::Left;
        }
    } else {
        if (direction.y() > 0) {
            return GraphLayout::GraphEdge::Down;
        } else {
            return GraphLayout::GraphEdge::Up;
        }
    }
}

static std::set<std::pair<ut64, ut64>> SelectLoopEdges(const GraphLayout::Graph &graph, ut64 entry)
{
    std::set<std::pair<ut64, ut64>> result;
    // Run DFS to select backwards/loop edges
    // 0 - not visited
    // 1 - in stack
    // 2 - visited
    std::unordered_map<ut64, uint8_t> visited;
    visited.reserve(graph.size());
    std::stack<std::pair<ut64, size_t>> stack;
    auto dfsFragment = [&visited, &graph, &stack, &result](ut64 first) {
        visited[first] = 1;
        stack.push({first, 0});
        while (!stack.empty()) {
            auto v = stack.top().first;
            auto edge_index = stack.top().second;
            auto blockIt = graph.find(v);
            if (blockIt == graph.end()) {
                continue;
            }
            const auto &block = blockIt->second;
            if (edge_index < block.edges.size()) {
                ++stack.top().second;
                auto target = block.edges[edge_index].target;
                auto &targetState = visited[target];
                if (targetState == 0) {
                    targetState = 1;
                    stack.push({target, 0});
                } else if (targetState == 1) {
                    result.insert({v, target});
                }
            } else {
                stack.pop();
                visited[v] = 2;
            }
        }
    };

    dfsFragment(entry);
    for (auto &blockIt : graph) {
        if (!visited[blockIt.first]) {
            dfsFragment(blockIt.first);
        }
    }

    return result;
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

    agsafeset(g, STR("splines"), lineType == LineType::Ortho ? STR("ortho") : STR("polyline"), STR(""));
    switch (direction) {
    case Direction::LR:
        agsafeset(g, STR("rankdir"), STR("LR"), STR(""));
        break;
    case Direction::TB:
        agsafeset(g, STR("rankdir"), STR("BT"), STR(""));
        break;
    }
    agsafeset(g, STR("newrank"), STR("true"), STR(""));
    // graphviz has builtin 72 dpi setting for input that differs from output
    // it's easier to use 72 everywhere
    const double dpi = 72.0;
    agsafeset(g, STR("dpi"), STR("72"), STR(""));

    auto widhAttr = agattr(g, AGNODE, STR("width"), STR("1"));
    auto heightAatr = agattr(g, AGNODE, STR("height"), STR("1"));
    agattr(g, AGNODE, STR("shape"), STR("box"));
    agattr(g, AGNODE, STR("fixedsize"), STR("true"));
    auto constraintAttr = agattr(g, AGEDGE, STR("constraint"), STR("1"));

    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    auto setFloatingPointAttr = [&stream](void *obj, Agsym_t *sym, double value) {
        stream.str({});
        stream << std::fixed << std::setw(4) << value;
        auto str = stream.str();
        agxset(obj, sym, STR(str.c_str()));
    };

    std::set<std::pair<ut64, ut64>> loopEdges = SelectLoopEdges(blocks, entry);

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
            if (loopEdges.find({blockIt.first, edge.target}) != loopEdges.end()) {
                agxset(e, constraintAttr, STR("0"));
            }
        }
        setFloatingPointAttr(u, widhAttr, block.width / dpi);
        setFloatingPointAttr(u, heightAatr, block.height / dpi);
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
                        }

                        if (edge.polyline.size() >= 2) {
                            // make sure self loops go from bottom to top
                            if (edge.target == block.entry && edge.polyline.first().y() < edge.polyline.last().y()) {
                                std::reverse(edge.polyline.begin(), edge.polyline.end());
                            }
                            auto it = std::prev(edge.polyline.end());
                            QPointF direction = *it;
                            direction -= *(--it);
                            edge.arrow = getArrowDirection(direction, lineType == LineType::Polyline);

                        } else {
                            edge.arrow = GraphEdge::Down;
                        }
                    }
                }
            }
        }
    }

    gvFreeLayout(gvc, g);
    agclose(g);
    gvFreeContext(gvc);
#undef STR
}
