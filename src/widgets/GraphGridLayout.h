#ifndef GRAPHGRIDLAYOUT_H
#define GRAPHGRIDLAYOUT_H

#include "core/Cutter.h"
#include "GraphLayout.h"
#include "common/LinkedListPool.h"


/**
 * @brief Graph layout algorithm on layered graph layout approach. For simplicity all the nodes are placed in a grid.
 */
class GraphGridLayout : public GraphLayout
{
public:
    enum class LayoutType {
        Medium,
        Wide,
        Narrow,
    };

    GraphGridLayout(LayoutType layoutType = LayoutType::Medium);
    virtual void CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks,
                                 ut64 entry,
                                 int &width,
                                 int &height) const override;
private:
    LayoutType layoutType;
    bool tightSubtreePlacement = false;
    bool parentBetweenDirectChild = false;

    struct GridBlock {
        ut64 id;
        std::vector<ut64> tree_edge; //< subset of outgoing edges that form a tree
        std::vector<ut64> dag_edge; //< subset of outgoing edges that form a tree
        std::size_t has_parent = false;
        int level = 0;
        int inputCount = 0;
        int outputCount = 0;

        /// Number of rows in subtree
        int row_count = 0;
        /// Column in which the block is
        int col = 0;
        /// Row in which the block is
        int row = 0;

        int lastRowWidth;
        int lastRowLeft;
        int leftPosition;
        int rightPosition;
        LinkedListPool<int>::List leftSide;
        LinkedListPool<int>::List rightSide;
    };

    struct Point {
        int row;
        int col;
        int offset;
        int16_t kind;
        int16_t spacingOverride;
    };

    struct GridEdge {
        ut64 dest;
        int mainColumn = -1;
        std::vector<Point> points;
        int secondaryPriority;

        void addPoint(int row, int col, int16_t kind = 0)
        {
            this->points.push_back({row, col, 0, kind, 0});
        }
    };

    struct LayoutState {
        std::unordered_map<ut64, GridBlock> grid_blocks;
        std::unordered_map<ut64, GraphBlock> *blocks = nullptr;
        std::unordered_map<ut64, std::vector<GridEdge>> edge;
        size_t rows = -1;
        size_t columns = -1;
        std::vector<int> columnWidth;
        std::vector<int> rowHeight;
        std::vector<int> edgeColumnWidth;
        std::vector<int> edgeRowHeight;

        std::vector<int> columnOffset;
        std::vector<int> rowOffset;
        std::vector<int> edgeColumnOffset;
        std::vector<int> edgeRowOffset;
    };

    using GridBlockMap = std::unordered_map<ut64, GridBlock>;

    void computeAllBlockPlacement(const std::vector<ut64> &blockOrder,
                                  LayoutState &layoutState) const;
    static std::vector<ut64> topoSort(LayoutState &state, ut64 entry);

    void routeEdges(LayoutState &state) const;
    void calculateEdgeMainColumn(LayoutState &state) const;
    void roughRouting(LayoutState &state) const;
    void elaborateEdgePlacement(LayoutState &state) const;
    void adjustColumnWidths(LayoutState &state) const;
    static int calculateColumnOffsets(const std::vector<int> &columnWidth, std::vector<int> &edgeColumnWidth,
                                      std::vector<int> &columnOffset, std::vector<int> &edgeColumnOffset);
    void convertToPixelCoordinates(LayoutState &state, int &width, int &height) const;
};

#endif // GRAPHGRIDLAYOUT_H
