#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include "core/Cutter.h"

#include <unordered_map>

class GraphLayout
{
public:
    struct GraphEdge {
        ut64 target;
        QPolygonF polyline;

        explicit GraphEdge(ut64 target): target(target) {}
    };

    struct GraphBlock {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        // This is a unique identifier, e.g. offset in the case of r2 blocks
        ut64 entry;
        // Edges
        std::vector<GraphEdge> edges;
    };

    struct LayoutConfig {
        int block_vertical_margin = 40;
        int block_horizontal_margin = 10;
    };

    GraphLayout(const LayoutConfig &layout_config) : layoutConfig(layout_config) {}
    virtual ~GraphLayout() {}
    virtual void CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry, int &width,
                                 int &height) const = 0;
protected:
    LayoutConfig layoutConfig;
};

#endif // GRAPHLAYOUT_H
