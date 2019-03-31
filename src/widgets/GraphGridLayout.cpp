#include "GraphGridLayout.h"

#include <unordered_set>
#include <queue>

// Vector functions
template<class T>
static void removeFromVec(std::vector<T> &vec, T elem)
{
    vec.erase(std::remove(vec.begin(), vec.end(), elem), vec.end());
}

GraphGridLayout::GraphGridLayout(GraphGridLayout::LayoutType layoutType)
    : GraphLayout({})
, layoutType(layoutType)
{
}

void GraphGridLayout::CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry,
                                      int &width, int &height) const
{
    // Populate incoming lists
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        for (auto &edge : block.exits) {
            blocks[edge].incoming.push_back(block.entry);
        }
    }

    std::unordered_set<ut64> visited;
    visited.insert(entry);
    std::queue<ut64> queue;
    std::vector<ut64> block_order;
    queue.push(entry);

    bool changed = true;
    while (changed) {
        changed = false;

        // Pick nodes with single entrypoints
        while (!queue.empty()) {
            GraphBlock &block = blocks[queue.front()];
            queue.pop();
            block_order.push_back(block.entry);
            for (ut64 edge : block.exits) {
                // Skip edge if we already visited it
                if (visited.count(edge)) {
                    continue;
                }

                // Some edges might not be available
                if (!blocks.count(edge)) {
                    continue;
                }

                // If this node has no other incoming edges, add it to the graph layout
                if (blocks[edge].incoming.size() == 1) {
                    removeFromVec(blocks[edge].incoming, block.entry);
                    block.new_exits.push_back(edge);
                    queue.push(blocks[edge].entry);
                    visited.insert(edge);
                    changed = true;
                } else {
                    // Remove from incoming edges
                    removeFromVec(blocks[edge].incoming, block.entry);
                }
            }
        }

        // No more nodes satisfy constraints, pick a node to continue constructing the graph
        ut64 best = 0;
        int best_edges;
        ut64 best_parent;
        for (auto &blockIt : blocks) {
            GraphBlock &block = blockIt.second;
            // Skip blocks we haven't visited yet
            if (!visited.count(block.entry)) {
                continue;
            }
            for (ut64 edge : block.exits) {
                // If we already visited the exit, skip it
                if (visited.count(edge)) {
                    continue;
                }
                if (!blocks.count(edge)) {
                    continue;
                }
                // find best edge
                if ((best == 0) || ((int)blocks[edge].incoming.size() < best_edges) || (
                            ((int)blocks[edge].incoming.size() == best_edges) && (edge < best))) {
                    best = edge;
                    best_edges = blocks[edge].incoming.size();
                    best_parent = block.entry;
                }
            }
        }
        if (best != 0) {
            GraphBlock &best_parentb = blocks[best_parent];
            removeFromVec(blocks[best].incoming, best_parentb.entry);
            best_parentb.new_exits.push_back(best);
            visited.insert(best);
            queue.push(best);
            changed = true;
        }
    }

    computeBlockPlacement(blocks[entry], blocks);

    // Prepare edge routing
    GraphBlock &entryb = blocks[entry];
    EdgesVector horiz_edges, vert_edges;
    horiz_edges.resize(entryb.row_count + 1);
    vert_edges.resize(entryb.row_count + 1);
    Matrix<bool> edge_valid;
    edge_valid.resize(entryb.row_count + 1);
    for (int row = 0; row < entryb.row_count + 1; row++) {
        horiz_edges[row].resize(entryb.col_count + 1);
        vert_edges[row].resize(entryb.col_count + 1);
        edge_valid[row].assign(entryb.col_count + 1, true);
    }

    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        edge_valid[block.row][block.col + 1] = false;
    }

    // Perform edge routing
    for (ut64 block_id : block_order) {
        GraphBlock &block = blocks[block_id];
        GraphBlock &start = block;
        for (ut64 edge : block.exits) {
            GraphBlock &end = blocks[edge];
            start.edges.push_back(routeEdge(horiz_edges, vert_edges, edge_valid, start, end));
        }
    }

    // Compute edge counts for each row and column
    std::vector<int> col_edge_count, row_edge_count;
    col_edge_count.assign(entryb.col_count + 1, 0);
    row_edge_count.assign(entryb.row_count + 1, 0);
    for (int row = 0; row < entryb.row_count + 1; row++) {
        for (int col = 0; col < entryb.col_count + 1; col++) {
            if (int(horiz_edges[row][col].size()) > row_edge_count[row])
                row_edge_count[row] = int(horiz_edges[row][col].size());
            if (int(vert_edges[row][col].size()) > col_edge_count[col])
                col_edge_count[col] = int(vert_edges[row][col].size());
        }
    }


    //Compute row and column sizes
    std::vector<int> col_width, row_height;
    col_width.assign(entryb.col_count + 1, 0);
    row_height.assign(entryb.row_count + 1, 0);
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        if ((int(block.width / 2)) > col_width[block.col])
            col_width[block.col] = int(block.width / 2);
        if ((int(block.width / 2)) > col_width[block.col + 1])
            col_width[block.col + 1] = int(block.width / 2);
        if (int(block.height) > row_height[block.row])
            row_height[block.row] = int(block.height);
    }

    // Compute row and column positions
    std::vector<int> col_x, row_y;
    col_x.assign(entryb.col_count, 0);
    row_y.assign(entryb.row_count, 0);
    std::vector<int> col_edge_x(entryb.col_count + 1);
    std::vector<int> row_edge_y(entryb.row_count + 1);
    int x = layoutConfig.block_horizontal_margin * 2;
    for (int i = 0; i < entryb.col_count; i++) {
        col_edge_x[i] = x;
        x += layoutConfig.block_horizontal_margin * col_edge_count[i];
        col_x[i] = x;
        x += col_width[i];
    }
    int y = layoutConfig.block_vertical_margin * 2;
    for (int i = 0; i < entryb.row_count; i++) {
        row_edge_y[i] = y;
        // TODO: The 1 when row_edge_count is 0 is not needed on the original.. not sure why it's required for us
        if (!row_edge_count[i]) {
            row_edge_count[i] = 1;
        }
        y += layoutConfig.block_vertical_margin * row_edge_count[i];
        row_y[i] = y;
        y += row_height[i];
    }
    col_edge_x[entryb.col_count] = x;
    row_edge_y[entryb.row_count] = y;
    width = x + (layoutConfig.block_horizontal_margin * 2) + (layoutConfig.block_horizontal_margin *
                                                              col_edge_count[entryb.col_count]);
    height = y + (layoutConfig.block_vertical_margin * 2) + (layoutConfig.block_vertical_margin *
                                                             row_edge_count[entryb.row_count]);

    //Compute node positions
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        block.x = int(
                      (col_x[block.col] + col_width[block.col] + ((layoutConfig.block_horizontal_margin / 2) *
                                                                  col_edge_count[block.col
                                                                                 + 1])) - (block.width / 2));
        if ((block.x + block.width) > (
                    col_x[block.col] + col_width[block.col] + col_width[block.col + 1] +
                    layoutConfig.block_horizontal_margin *
                    col_edge_count[
                 block.col + 1])) {
            block.x = int((col_x[block.col] + col_width[block.col] + col_width[block.col + 1] +
                           layoutConfig.block_horizontal_margin * col_edge_count[
                    block.col + 1]) - block.width);
        }
        block.y = row_y[block.row];
    }

    // Precompute coordinates for edges
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        for (GraphEdge &edge : block.edges) {
            auto start = edge.points[0];
            auto start_col = start.col;
            auto last_index = edge.start_index;
            // This is the start point of the edge.
            auto first_pt = QPoint(col_edge_x[start_col] + (layoutConfig.block_horizontal_margin * last_index) +
                                   (layoutConfig.block_horizontal_margin / 2),
                                   block.y + block.height);
            auto last_pt = first_pt;
            QPolygonF pts;
            pts.append(last_pt);

            for (int i = 0; i < int(edge.points.size()); i++) {
                auto end = edge.points[i];
                auto end_row = end.row;
                auto end_col = end.col;
                auto last_index = end.index;
                QPoint new_pt;
                // block_vertical_margin/2 gives the margin from block to the horizontal lines
                if (start_col == end_col)
                    new_pt = QPoint(last_pt.x(), row_edge_y[end_row] + (layoutConfig.block_vertical_margin * last_index)
                                    +
                                    (layoutConfig.block_vertical_margin / 2));
                else
                    new_pt = QPoint(col_edge_x[end_col] + (layoutConfig.block_horizontal_margin * last_index) +
                                    (layoutConfig.block_horizontal_margin / 2), last_pt.y());
                pts.push_back(new_pt);
                last_pt = new_pt;
                start_col = end_col;
            }


            auto new_pt = QPoint(last_pt.x(), edge.dest->y - 1);
            pts.push_back(new_pt);
            edge.polyline = pts;
        }
    }
}


// Prepare graph
// This computes the position and (row/col based) size of the block
// Recursively calls itself for each child of the GraphBlock
void GraphGridLayout::computeBlockPlacement(GraphBlock &block,
                                            std::unordered_map<ut64, GraphBlock> &blocks) const
{
    int col = 0;
    int row_count = 1;
    int childColumn = 0;
    bool singleChild = block.new_exits.size() == 1;
    // Compute all children nodes
    for (size_t i = 0; i < block.new_exits.size(); i++) {
        ut64 edge = block.new_exits[i];
        GraphBlock &edgeb = blocks[edge];
        computeBlockPlacement(edgeb, blocks);
        row_count = std::max(edgeb.row_count + 1, row_count);
        childColumn = edgeb.col;
    }

    if (layoutType != LayoutType::Wide && block.new_exits.size() == 2) {
        GraphBlock &left = blocks[block.new_exits[0]];
        GraphBlock &right = blocks[block.new_exits[1]];
        if (left.new_exits.size() == 0) {
            left.col = right.col - 2;
            int add = left.col < 0 ? - left.col : 0;
            adjustGraphLayout(right, blocks, add, 1);
            adjustGraphLayout(left, blocks, add, 1);
            col = right.col_count + add;
        } else if (right.new_exits.size() == 0) {
            adjustGraphLayout(left, blocks, 0, 1);
            adjustGraphLayout(right, blocks, left.col + 2, 1);
            col = std::max(left.col_count, right.col + 2);
        } else {
            adjustGraphLayout(left, blocks, 0, 1);
            adjustGraphLayout(right, blocks, left.col_count, 1);
            col = left.col_count + right.col_count;
        }
        block.col_count = std::max(2, col);
        if (layoutType == LayoutType::Medium) {
            block.col = (left.col + right.col) / 2;
        } else {
            block.col = singleChild ? childColumn : (col - 2) / 2;
        }
    } else {
        for (ut64 edge : block.new_exits) {
            adjustGraphLayout(blocks[edge], blocks, col, 1);
            col += blocks[edge].col_count;
        }
        if (col >= 2) {
            // Place this node centered over the child nodes
            block.col = singleChild ? childColumn : (col - 2) / 2;
            block.col_count = col;
        } else {
            //No child nodes, set single node's width (nodes are 2 columns wide to allow
            //centering over a branch)
            block.col = 0;
            block.col_count = 2;
        }
    }
    block.row = 0;
    block.row_count = row_count;
}

void GraphGridLayout::adjustGraphLayout(GraphBlock &block,
                                        std::unordered_map<ut64, GraphBlock> &blocks, int col, int row) const
{
    block.col += col;
    block.row += row;
    for (ut64 edge : block.new_exits) {
        adjustGraphLayout(blocks[edge], blocks, col, row);
    }
}

// Edge computing stuff
bool GraphGridLayout::isEdgeMarked(EdgesVector &edges, int row, int col, int index)
{
    if (index >= int(edges[row][col].size()))
        return false;
    return edges[row][col][index];
}

void GraphGridLayout::markEdge(EdgesVector &edges, int row, int col, int index, bool used)
{
    while (int(edges[row][col].size()) <= index)
        edges[row][col].push_back(false);
    edges[row][col][index] = used;
}

GraphGridLayout::GraphEdge GraphGridLayout::routeEdge(EdgesVector &horiz_edges,
                                                      EdgesVector &vert_edges,
                                                      Matrix<bool> &edge_valid, GraphBlock &start, GraphBlock &end) const
{
    GraphEdge edge;
    edge.dest = &end;

    //Find edge index for initial outgoing line
    int i = 0;
    while (true) {
        if (!isEdgeMarked(vert_edges, start.row + 1, start.col + 1, i))
            break;
        i += 1;
    }
    markEdge(vert_edges, start.row + 1, start.col + 1, i);
    edge.addPoint(start.row + 1, start.col + 1);
    edge.start_index = i;
    bool horiz = false;

    //Find valid column for moving vertically to the target node
    int min_row, max_row;
    if (end.row < (start.row + 1)) {
        min_row = end.row;
        max_row = start.row + 1;
    } else {
        min_row = start.row + 1;
        max_row = end.row;
    }
    int col = start.col + 1;
    if (min_row != max_row) {
        auto checkColumn = [min_row, max_row, &edge_valid](int column) {
            if (column < 0 || column >= int(edge_valid[min_row].size()))
                return false;
            for (int row = min_row; row < max_row; row++) {
                if (!edge_valid[row][column]) {
                    return false;
                }
            }
            return true;
        };

        if (!checkColumn(col)) {
            if (checkColumn(end.col + 1)) {
                col = end.col + 1;
            } else {
                int ofs = 0;
                while (true) {
                    col = start.col + 1 - ofs;
                    if (checkColumn(col)) {
                        break;
                    }

                    col = start.col + 1 + ofs;
                    if (checkColumn(col)) {
                        break;
                    }

                    ofs += 1;
                }
            }
        }
    }

    if (col != (start.col + 1)) {
        //Not in same column, need to generate a line for moving to the correct column
        int min_col, max_col;
        if (col < (start.col + 1)) {
            min_col = col;
            max_col = start.col + 1;
        } else {
            min_col = start.col + 1;
            max_col = col;
        }
        int index = findHorizEdgeIndex(horiz_edges, start.row + 1, min_col, max_col);
        edge.addPoint(start.row + 1, col, index);
        horiz = true;
    }

    if (end.row != (start.row + 1)) {
        //Not in same row, need to generate a line for moving to the correct row
        if (col == (start.col + 1))
            markEdge(vert_edges, start.row + 1, start.col + 1, i, false);
        int index = findVertEdgeIndex(vert_edges, col, min_row, max_row);
        if (col == (start.col + 1))
            edge.start_index = index;
        edge.addPoint(end.row, col, index);
        horiz = false;
    }

    if (col != (end.col + 1)) {
        //Not in ending column, need to generate a line for moving to the correct column
        int min_col, max_col;
        if (col < (end.col + 1)) {
            min_col = col;
            max_col = end.col + 1;
        } else {
            min_col = end.col + 1;
            max_col = col;
        }
        int index = findHorizEdgeIndex(horiz_edges, end.row, min_col, max_col);
        edge.addPoint(end.row, end.col + 1, index);
        horiz = true;
    }

    //If last line was horizontal, choose the ending edge index for the incoming edge
    if (horiz) {
        int index = findVertEdgeIndex(vert_edges, end.col + 1, end.row, end.row);
        edge.points[int(edge.points.size()) - 1].index = index;
    }

    return edge;
}


int GraphGridLayout::findHorizEdgeIndex(EdgesVector &edges, int row, int min_col, int max_col)
{
    //Find a valid index
    int i = 0;
    while (true) {
        bool valid = true;
        for (int col = min_col; col < max_col + 1; col++)
            if (isEdgeMarked(edges, row, col, i)) {
                valid = false;
                break;
            }
        if (valid)
            break;
        i++;
    }

    //Mark chosen index as used
    for (int col = min_col; col < max_col + 1; col++)
        markEdge(edges, row, col, i);
    return i;
}

int GraphGridLayout::findVertEdgeIndex(EdgesVector &edges, int col, int min_row, int max_row)
{
    //Find a valid index
    int i = 0;
    while (true) {
        bool valid = true;
        for (int row = min_row; row < max_row + 1; row++)
            if (isEdgeMarked(edges, row, col, i)) {
                valid = false;
                break;
            }
        if (valid)
            break;
        i++;
    }

    //Mark chosen index as used
    for (int row = min_row; row < max_row + 1; row++)
        markEdge(edges, row, col, i);
    return i;
}
