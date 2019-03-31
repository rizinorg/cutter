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
    virtual void CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry, int &width,
                                 int &height) const override;
private:
    LayoutType layoutType;

    void computeBlockPlacement(GraphBlock &block,
                               std::unordered_map<ut64, GraphBlock> &blocks) const;
    void adjustGraphLayout(GraphBlock &block, std::unordered_map<ut64, GraphBlock> &blocks, int col,
                           int row) const; //TODO: improve name/description

    // Edge computing stuff
    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;

    GraphEdge routeEdge(EdgesVector &horiz_edges, EdgesVector &vert_edges,
                        Matrix<bool> &edge_valid, GraphBlock &start, GraphBlock &end) const;
    static int findVertEdgeIndex(EdgesVector &edges, int col, int min_row, int max_row);
    static bool isEdgeMarked(EdgesVector &edges, int row, int col, int index);
    static void markEdge(EdgesVector &edges, int row, int col, int index, bool used = true);
    static int findHorizEdgeIndex(EdgesVector &edges, int row, int min_col, int max_col);
};

#endif // GRAPHGRIDLAYOUT_H
