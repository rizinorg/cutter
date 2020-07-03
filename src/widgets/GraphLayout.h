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
        enum ArrowDirection {
            Down, Left, Up, Right, None
        };
        ArrowDirection arrow = ArrowDirection::Down;

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
    using Graph = std::unordered_map<ut64, GraphBlock>;

    struct LayoutConfig {
        int blockVerticalSpacing = 40;
        int blockHorizontalSpacing = 20;
        int edgeVerticalSpacing = 10;
        int edgeHorizontalSpacing = 10;
    };

    GraphLayout(const LayoutConfig &layout_config) : layoutConfig(layout_config) {}
    virtual ~GraphLayout() {}
    virtual void CalculateLayout(Graph &blocks, ut64 entry, int &width,
                                 int &height) const = 0;
    virtual void setLayoutConfig(const LayoutConfig &config)
    {
        this->layoutConfig = config;
    };
protected:
    LayoutConfig layoutConfig;
};

#endif // GRAPHLAYOUT_H
