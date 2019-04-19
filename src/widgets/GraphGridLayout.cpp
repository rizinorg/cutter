#include "GraphGridLayout.h"

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <cassert>

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

std::vector<ut64> GraphGridLayout::topoSort(LayoutState &state, ut64 entry)
{
    auto &blocks = *state.blocks;

    // Run DFS to:
    // * select backwards/loop edges
    // * perform toposort
    std::vector<ut64> blockOrder;
    // 0 - not visited
    // 1 - in stack
    // 2 - visited
    std::unordered_map<ut64, uint8_t> visited;
    visited.reserve(state.blocks->size());
    std::stack<std::pair<ut64, size_t>> stack;
    auto dfsFragment = [&visited, &blocks, &state, &stack, &blockOrder](ut64 first) {
        visited[first] = 1;
        stack.push({first, 0});
        while (!stack.empty()) {
            auto v = stack.top().first;
            auto edge_index = stack.top().second;
            const auto &block = blocks[v];
            if (edge_index < block.edges.size()) {
                ++stack.top().second;
                auto target = block.edges[edge_index].target;
                auto &targetState = visited[target];
                if (targetState == 0) {
                    targetState = 1;
                    stack.push({target, 0});
                    state.grid_blocks[v].dag_edge.push_back(target);
                } else if (targetState == 2) {
                    state.grid_blocks[v].dag_edge.push_back(target);
                } // else {  targetState == 1 in stack, loop edge }
            } else {
                stack.pop();
                visited[v] = 2;
                blockOrder.push_back(v);
            }
        }
    };

    // Start with entry so that if start of function block is part of loop it
    // is still kept at top unless it's impossible to do while maintaining
    // topological order.
    dfsFragment(entry);
    for (auto &blockIt : blocks) {
        if (!visited[blockIt.first]) {
            dfsFragment(blockIt.first);
        }
    }

    // assign levels and select tree edges
    for (auto it = blockOrder.rbegin(), end = blockOrder.rend(); it != end; it++) {
        auto &block = state.grid_blocks[*it];
        int nextLevel = block.level + 1;
        for (auto target : block.dag_edge) {
            auto &targetBlock = state.grid_blocks[target];
            targetBlock.level = std::max(targetBlock.level, nextLevel);
        }
    }
    for (auto &blockIt : state.grid_blocks) {
        auto &block = blockIt.second;
        for (auto targetId : block.dag_edge) {
            auto &targetBlock = state.grid_blocks[targetId];
            if (!targetBlock.has_parent && targetBlock.level == block.level + 1) {
                block.tree_edge.push_back(targetId);
                targetBlock.has_parent = true;
            }
        }
    }
    return blockOrder;
}

void GraphGridLayout::CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks, ut64 entry,
                                      int &width, int &height) const
{
    LayoutState layoutState;
    layoutState.blocks = &blocks;

    for (auto &it : blocks) {
        GridBlock block;
        block.id = it.first;
        layoutState.grid_blocks[it.first] = block;
    }

    auto block_order = topoSort(layoutState, entry);
    computeAllBlockPlacement(block_order, layoutState);

    for (auto &blockIt : blocks) {
        layoutState.edge[blockIt.first].resize(blockIt.second.edges.size());
    }

    // Prepare edge routing
    int col_count = 1;
    int row_count = 0;
    for (const auto &blockIt : layoutState.grid_blocks) {
        if (!blockIt.second.has_parent) {
            row_count = std::max(row_count, blockIt.second.row_count);
            col_count += blockIt.second.col_count;
        }
    }
    row_count += 2;
    EdgesVector horiz_edges, vert_edges;
    horiz_edges.resize(row_count + 1);
    vert_edges.resize(row_count + 1);
    Matrix<bool> edge_valid;
    edge_valid.resize(row_count + 1);
    for (int row = 0; row < row_count + 1; row++) {
        horiz_edges[row].resize(col_count + 1);
        vert_edges[row].resize(col_count + 1);
        edge_valid[row].assign(col_count + 1, true);
    }

    for (auto &blockIt : layoutState.grid_blocks) {
        auto &block = blockIt.second;
        edge_valid[block.row][block.col + 1] = false;
    }

    // Perform edge routing
    for (ut64 blockId : block_order) {
        GraphBlock &block = blocks[blockId];
        GridBlock &start = layoutState.grid_blocks[blockId];
        size_t i = 0;
        for (const auto &edge : block.edges) {
            GridBlock &end = layoutState.grid_blocks[edge.target];
            layoutState.edge[blockId][i++] = routeEdge(horiz_edges, vert_edges, edge_valid, start, end);
        }
    }

    // Compute edge counts for each row and column
    std::vector<int> col_edge_count, row_edge_count;
    col_edge_count.assign(col_count + 1, 0);
    row_edge_count.assign(row_count + 1, 0);
    for (int row = 0; row < row_count + 1; row++) {
        for (int col = 0; col < col_count + 1; col++) {
            if (int(horiz_edges[row][col].size()) > row_edge_count[row])
                row_edge_count[row] = int(horiz_edges[row][col].size());
            if (int(vert_edges[row][col].size()) > col_edge_count[col])
                col_edge_count[col] = int(vert_edges[row][col].size());
        }
    }


    //Compute row and column sizes
    std::vector<int> col_width, row_height;
    col_width.assign(col_count + 1, 0);
    row_height.assign(row_count + 1, 0);
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        GridBlock &grid_block = layoutState.grid_blocks[blockIt.first];
        if ((int(block.width / 2)) > col_width[grid_block.col])
            col_width[grid_block.col] = int(block.width / 2);
        if ((int(block.width / 2)) > col_width[grid_block.col + 1])
            col_width[grid_block.col + 1] = int(block.width / 2);
        if (int(block.height) > row_height[grid_block.row])
            row_height[grid_block.row] = int(block.height);
    }

    // Compute row and column positions
    std::vector<int> col_x, row_y;
    col_x.assign(col_count, 0);
    row_y.assign(row_count, 0);
    std::vector<int> col_edge_x(col_count + 1);
    std::vector<int> row_edge_y(row_count + 1);
    int x = layoutConfig.block_horizontal_margin;
    for (int i = 0; i <= col_count; i++) {
        col_edge_x[i] = x;
        x += layoutConfig.block_horizontal_margin * col_edge_count[i];
        if (i < col_count) {
            col_x[i] = x;
            x += col_width[i];
        }
    }
    int y = layoutConfig.block_vertical_margin;
    for (int i = 0; i <= row_count; i++) {
        row_edge_y[i] = y;
        if (!row_edge_count[i]) {
            // prevent 2 blocks being put on top of each other without any space
            row_edge_count[i] = 1;
        }
        y += layoutConfig.block_vertical_margin * row_edge_count[i];
        if (i < row_count) {
            row_y[i] = y;
            y += row_height[i];
        }
    }
    width = x + (layoutConfig.block_horizontal_margin);
    height = y + (layoutConfig.block_vertical_margin);

    //Compute node positions
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        GridBlock &grid_block = layoutState.grid_blocks[blockIt.first];
        auto column = grid_block.col;
        auto row = grid_block.row;
        block.x = int(col_x[column] + col_width[column] +
                      ((layoutConfig.block_horizontal_margin / 2) * col_edge_count[column + 1])
                      - (block.width / 2));
        if ((block.x + block.width) > (
                    col_x[column] + col_width[column] + col_width[column + 1] +
                    layoutConfig.block_horizontal_margin *
                    col_edge_count[column + 1])) {
            block.x = int((col_x[column] + col_width[column] + col_width[column + 1] +
                           layoutConfig.block_horizontal_margin * col_edge_count[column + 1]) - block.width);
        }
        block.y = row_y[row];
    }

    // Compute coordinates for edges
    auto position_from_middle = [](int index, int spacing, int column_count) {
        return spacing * (((index & 1) ? 1 : -1) * ((index + 1) / 2) + (column_count - 1) / 2);
    };
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        size_t index = 0;
        assert(block.edges.size() == layoutState.edge[block.entry].size());
        for (GridEdge &edge : layoutState.edge[block.entry]) {
            if (edge.points.empty()) {
                qDebug() << "Warning, unrouted edge.";
                continue;
            }
            auto start = edge.points[0];
            auto start_col = start.col;
            auto last_index = edge.start_index;
            // This is the start point of the edge.
            auto first_pt = QPoint(col_edge_x[start_col] +
                                   position_from_middle(last_index, layoutConfig.block_horizontal_margin, col_edge_count[start_col]) +
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
                    new_pt = QPoint(last_pt.x(), row_edge_y[end_row] +
                                    position_from_middle(last_index, layoutConfig.block_vertical_margin, row_edge_count[end_row]) +
                                    (layoutConfig.block_vertical_margin / 2));
                else
                    new_pt = QPoint(col_edge_x[end_col] +
                                    position_from_middle(last_index, layoutConfig.block_horizontal_margin, col_edge_count[end_col]) +
                                    (layoutConfig.block_horizontal_margin / 2), last_pt.y());
                pts.push_back(new_pt);
                last_pt = new_pt;
                start_col = end_col;
            }

            const auto &target = blocks[edge.dest];
            auto new_pt = QPoint(last_pt.x(), target.y - 1);
            pts.push_back(new_pt);
            block.edges[index].polyline = pts;
            index++;
        }
    }
}

void GraphGridLayout::computeAllBlockPlacement(const std::vector<ut64> &blockOrder,
                                               LayoutState &layoutState) const
{
    for (auto blockId : blockOrder) {
        computeBlockPlacement(blockId, layoutState);
    }
    int col = 0;
    for (auto blockId : blockOrder) {
        if (!layoutState.grid_blocks[blockId].has_parent) {
            adjustGraphLayout(layoutState.grid_blocks[blockId], layoutState.grid_blocks, col, 1);
            col += layoutState.grid_blocks[blockId].col_count;
        }
    }
}

// Prepare graph
// This computes the position and (row/col based) size of the block
void GraphGridLayout::computeBlockPlacement(ut64 blockId, LayoutState &layoutState) const
{
    auto &block = layoutState.grid_blocks[blockId];
    auto &blocks = layoutState.grid_blocks;
    int col = 0;
    int row_count = 1;
    int childColumn = 0;
    bool singleChild = block.tree_edge.size() == 1;
    // Compute all children nodes
    for (size_t i = 0; i < block.tree_edge.size(); i++) {
        ut64 edge = block.tree_edge[i];
        auto &edgeb = blocks[edge];
        row_count = std::max(edgeb.row_count + 1, row_count);
        childColumn = edgeb.col;
    }

    if (layoutType != LayoutType::Wide && block.tree_edge.size() == 2) {
        auto &left = blocks[block.tree_edge[0]];
        auto &right = blocks[block.tree_edge[1]];
        if (left.tree_edge.size() == 0) {
            left.col = right.col - 2;
            int add = left.col < 0 ? - left.col : 0;
            adjustGraphLayout(right, blocks, add, 1);
            adjustGraphLayout(left, blocks, add, 1);
            col = right.col_count + add;
        } else if (right.tree_edge.size() == 0) {
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
        for (ut64 edge : block.tree_edge) {
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

void GraphGridLayout::adjustGraphLayout(GridBlock &block,
                                        std::unordered_map<ut64, GridBlock> &blocks, int col, int row) const
{
    block.col += col;
    block.row += row;
    for (ut64 edge : block.tree_edge) {
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

GraphGridLayout::GridEdge GraphGridLayout::routeEdge(EdgesVector &horiz_edges,
                                                     EdgesVector &vert_edges,
                                                     Matrix<bool> &edge_valid, GridBlock &start, GridBlock &end) const
{
    GridEdge edge;
    edge.dest = end.id;

    //Find edge index for initial outgoing line
    int i = 0;
    while (isEdgeMarked(vert_edges, start.row + 1, start.col + 1, i)) {
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
