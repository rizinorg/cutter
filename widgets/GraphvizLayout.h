#ifndef GRAPHVIZLAYOUT_H
#define GRAPHVIZLAYOUT_H

#include "core/Cutter.h"
#include "GraphLayout.h"

class GraphvizLayout : public GraphLayout
{
public:
    enum class LineType {
        Ortho,
        Polyline
    };
    enum class Direction {
        TB,
        LR
    };
    GraphvizLayout(LineType lineType, Direction direction = Direction::TB);
    virtual void CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks,
                                 ut64 entry,
                                 int &width,
                                 int &height) const override;
private:
    Direction direction;
    LineType lineType;
};

#endif // GRAPHVIZLAYOUT_H
