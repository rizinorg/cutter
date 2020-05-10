#ifndef GRAPHGRIDLAYOUT_H
#define GRAPHGRIDLAYOUT_H

#include "core/Cutter.h"
#include "GraphLayout.h"


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

    struct GridBlock {
        ut64 id;
        std::vector<ut64> tree_edge; //< subset of outgoing edges that form a tree
        std::vector<ut64> dag_edge; //< subset of outgoing edges that form a tree
        std::size_t has_parent = false;
        int level = 0;

        /// Number of rows in subtree
        int row_count = 0;
        /// Number of columns in subtree
        int col_count = 0;
        /// Column in which the block is
        int col = 0;
        /// Row in which the block is
        int row = 0;
    };

    struct Point {
        int row;
        int col;
        int kind;
        int offset;
    };

    struct GridEdge {
        ut64 dest;
        int mainColumn = -1;
        std::vector<Point> points;

        void addPoint(int row, int col, int kind = 0)
        {
            this->points.push_back({row, col, kind, 0});
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
    void computeBlockPlacement(ut64 blockId,
                               LayoutState &layoutState) const;
    void adjustGraphLayout(GridBlock &block, GridBlockMap &blocks,
                           int col, int row) const;
    static std::vector<ut64> topoSort(LayoutState &state, ut64 entry);

    void routeEdges(LayoutState &state) const;
    void calculateEdgeMainColumn(LayoutState &state) const;
    void roughRouting(LayoutState &state) const;
    void elaborateEdgePlacement(LayoutState &state) const;
    static int calculateColumnOffsets(const std::vector<int> &columnWidth, const std::vector<int> &edgeColumnWidth,
                                      std::vector<int> &columnOffset, std::vector<int> &edgeColumnOffset);
    void convertToPixelCoordinates(LayoutState &state, int &width, int &height) const;
};

#endif // GRAPHGRIDLAYOUT_H
