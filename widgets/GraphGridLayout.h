#ifndef GRAPHGRIDLAYOUT_H
#define GRAPHGRIDLAYOUT_H

#include "core/Cutter.h"
#include "GraphLayout.h"

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
        std::vector<ut64> tree_edge; // subset of outgoing edges that form a tree
        std::vector<ut64> dag_edge; // subset of outgoing edges that form a tree
        std::size_t has_parent = false;
        int level = 0;

        // Number of rows in block
        int row_count = 0;
        // Number of columns in block
        int col_count = 0;
        // Column in which the block is
        int col = 0;
        // Row in which the block is
        int row = 0;
    };

    struct Point {
        int row; //point[0]
        int col; //point[1]
        int index; //point[2]
    };

    struct GridEdge {
        ut64 dest;
        std::vector<Point> points;
        int start_index = 0;
        QPolygonF polyline;

        void addPoint(int row, int col, int index = 0)
        {
            Point point = {row, col, 0};
            this->points.push_back(point);
            if (int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct LayoutState {
        std::unordered_map<ut64, GridBlock> grid_blocks;
        std::unordered_map<ut64, GraphBlock> *blocks = nullptr;
        std::unordered_map<ut64, std::vector<GridEdge>> edge;
    };

    using GridBlockMap = std::unordered_map<ut64, GridBlock>;

    void computeAllBlockPlacement(const std::vector<ut64> &blockOrder,
                                  LayoutState &layoutState) const;
    void computeBlockPlacement(ut64 blockId,
                               LayoutState &layoutState) const;
    void adjustGraphLayout(GridBlock &block, GridBlockMap &blocks,
                           int col, int row) const;
    static std::vector<ut64> topoSort(LayoutState &state, ut64 entry);

    // Edge computing stuff
    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;

    GridEdge routeEdge(EdgesVector &horiz_edges, EdgesVector &vert_edges,
                       Matrix<bool> &edge_valid, GridBlock &start, GridBlock &end) const;
    static int findVertEdgeIndex(EdgesVector &edges, int col, int min_row, int max_row);
    static bool isEdgeMarked(EdgesVector &edges, int row, int col, int index);
    static void markEdge(EdgesVector &edges, int row, int col, int index, bool used = true);
    static int findHorizEdgeIndex(EdgesVector &edges, int row, int min_col, int max_col);
};

#endif // GRAPHGRIDLAYOUT_H
