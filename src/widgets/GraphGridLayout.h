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
    virtual void CalculateLayout(Graph &blocks,
                                 ut64 entry,
                                 int &width,
                                 int &height) const override;
    void setTightSubtreePlacement(bool enabled) { tightSubtreePlacement = enabled; }
    void setParentBetweenDirectChild(bool enabled) { parentBetweenDirectChild = enabled; }
    void setverticalBlockAlignmentMiddle(bool enabled) { verticalBlockAlignmentMiddle = enabled; }
    void setLayoutOptimization(bool enabled) { useLayoutOptimization = enabled; }
private:
    /// false - use bounding box for smallest subtree when placing them side by side
    bool tightSubtreePlacement = false;
    /// true if code should try to place parent between direct children as much as possible
    bool parentBetweenDirectChild = false;
    /// false if blocks in rows should be aligned at top, true for middle alignment
    bool verticalBlockAlignmentMiddle = false;
    bool useLayoutOptimization = true;

    struct GridBlock {
        ut64 id;
        std::vector<ut64> tree_edge; //!< subset of outgoing edges that form a tree
        std::vector<ut64> dag_edge; //!< subset of outgoing edges that form a dag
        std::size_t has_parent = false;
        int inputCount = 0;
        int outputCount = 0;

        /// Number of rows in subtree
        int row_count = 0;
        /// Column in which the block is
        int col = 0;
        /// Row in which the block is
        int row = 0;

        ut64 mergeBlock = 0;

        int lastRowLeft; //!< left side of subtree last row
        int lastRowRight; //!< right side of subtree last row
        int leftPosition; //!< left side of subtree
        int rightPosition; //!< right side of subtree
        LinkedListPool<int>::List leftSideShape;
        LinkedListPool<int>::List rightSideShape;
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

    /**
     * @brief Find nodes where control flow merges after splitting.
     * Sets node column offset so that after computing placement merge point is centered bellow nodes above.
     */
    void findMergePoints(LayoutState &state) const;
    /**
     * @brief Compute node rows and columns within grid.
     * @param blockOrder Nodes in the reverse topological order.
     */
    void computeAllBlockPlacement(const std::vector<ut64> &blockOrder,
                                  LayoutState &layoutState) const;
    /**
     * @brief Perform the topological sorting of graph nodes.
     * If the graph contains loops, a subset of edges is selected. Subset of edges forming DAG are stored in
     * GridBlock::dag_edge.
     * @param state Graph layout state including the input graph.
     * @param entry Entrypoint node. When removing loops prefer placing this node at top.
     * @return Reverse topological ordering.
     */
    static std::vector<ut64> topoSort(LayoutState &state, ut64 entry);

    /**
     * @brief Assign row positions to nodes.
     * @param state
     * @param blockOrder reverse topological ordering of nodes
     */
    static void assignRows(LayoutState &state, const std::vector<ut64> &blockOrder);
    /**
     * @brief Select subset of DAG edges that form tree.
     * @param state
     */
    static void selectTree(LayoutState &state);

    /**
     * @brief routeEdges Route edges, expects node positions to be calculated previously.
     */
    void routeEdges(LayoutState &state) const;
    /**
     * @brief Choose which column to use for transition from start node row to target node row.
     */
    void calculateEdgeMainColumn(LayoutState &state) const;
    /**
     * @brief Do rough edge routing within grid using up to 5 segments.
     */
    void roughRouting(LayoutState &state) const;
    /**
     * @brief Calculate segment placement relative to their columns.
     */
    void elaborateEdgePlacement(LayoutState &state) const;
    /**
     * @brief Recalculate column widths, trying to compensate for the space taken by edge columns.
     */
    void adjustColumnWidths(LayoutState &state) const;
    /**
     * @brief Calculate position of each column(or row) based on widths.
     * It is assumed that columnWidth.size() + 1 = edgeColumnWidth.size() and they are interleaved.
     * @param columnWidth
     * @param edgeColumnWidth
     * @param columnOffset
     * @param edgeColumnOffset
     * @return total width of all the columns
     */
    static int calculateColumnOffsets(const std::vector<int> &columnWidth, std::vector<int> &edgeColumnWidth,
                                      std::vector<int> &columnOffset, std::vector<int> &edgeColumnOffset);
    /**
     * @brief Final graph layout step. Convert grids cell relative positions to absolute pixel positions.
     * @param state
     * @param width image width output argument
     * @param height image height output argument
     */
    void convertToPixelCoordinates(LayoutState &state, int &width, int &height) const;
    /**
     * @brief Move the graph content to top left corner and update dimensions.
     * @param graph
     * @param width width after cropping
     * @param height height after cropping
     */
    void cropToContent(Graph &graph, int &width, int &height) const;
    /**
     * @brief Connect edge ends to blocks by changing y.
     * @param graph
     */
    void connectEdgeEnds(Graph &graph) const;
    /**
     * @brief Reduce spacing between nodes and edges by pushing everything together ignoring the grid.
     * @param state
     */
    void optimizeLayout(LayoutState &state) const;
};

#endif // GRAPHGRIDLAYOUT_H
