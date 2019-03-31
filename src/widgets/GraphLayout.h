#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include "core/Cutter.h"

#include <unordered_map>

class GraphLayout
{
public:
    struct GraphBlock;
    // TODO: move look and use case specific fields to GraphView and XGraphView
    // TODO: move GridLayout specific fields to GraphGridLayout
    struct Point {
        int row; //point[0]
        int col; //point[1]
        int index; //point[2]
    };

    struct GraphEdge {
        QColor color;
        GraphBlock *dest;
        std::vector<Point> points;
        int start_index = 0;

        QPolygonF polyline;
        QPolygonF arrow_start;
        QPolygonF arrow_end;

        void addPoint(int row, int col, int index = 0)
        {
            Point point = {row, col, 0};
            this->points.push_back(point);
            if (int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct GraphBlock {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        // This is a unique identifier, e.g. offset in the case of r2 blocks
        ut64 entry;
        // This contains unique identifiers to entries
        // Outgoing edges
        std::vector<ut64> exits;
        // Incoming edges
        std::vector<ut64> incoming;
        // TODO what is this
        std::vector<ut64> new_exits;

        // Number of rows in block
        int row_count = 0;
        // Number of columns in block
        int col_count = 0;
        // Column in which the block is
        int col = 0;
        // Row in which the block is
        int row = 0;

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
