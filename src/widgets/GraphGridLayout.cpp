#include "GraphGridLayout.h"

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <cassert>

/** @class GraphGridLayout

Basic familarity with graph algorithms is recommended.

# Terms used:
- **Vertice**, **node**, **block** - read description of graph for definition. Within this text vertice and node are
used interchangably with block due to code being written for visualizing basic block controll flow graph.
- **edge** - read description of graph for definition for precise definition.
- **DAG** - directed acyclic graph, graph using directed edges which doesn't have cycles. DAG may contain loops if
following them would require going in both directions of edges. Example 1->2 1->3 3->2 is a DAG, 2->1 1->3 3->2
isn't a DAG.
- **DFS** - depth first search, a graph traversal algorithm
- **toposort** - toplogical sorting, process of ordering a DAG vertices that all edges go from vertices erlier in the
toposort order to vertices later in toposort order. There are multiple algorithms for implementing toposort operation.
Single DAG can have multiple valid topoligical orderings, a toposort algorithm can be designed to priotarize a specific
one from all valid toposort orders. Example: for graph 1->4, 2->1, 2->3, 3->4 valid topological orders are [2,1,3,4] and
[2,3,1,4].

# High level strucutre of the algorithm
1. select subset of edges that form a DAG (remove cycles)
2. toposort the DAG
3. choose a subset of edges that form a tree and assign layers
4. assign node positions within grid using tree structure, child subtrees are placed side by side with parent on top
5. perform edge routing
6. calculate column and row pixel positions based on node sizes and amount edges between the rows


Contrary to many other layered graph drawing algorithm this implementation doesn't perform node reording to minimize
edge crossing. This simplifies implementation, and preserves original control flow structure for conditional jumps (
true jump on one side, false jump on other). Due to most of control flow being result of structured programming
constructs like if/then/else and loops, resulting layout is usually readable without node reordering within layers.


# Describtion of grid.
To simplify the layout algorithm initial steps assume that all nodes have the same size and edges are zero width.
After placing the nodes and routing the edges it is known which nodes are in in which row and column, how
many edges are between each pair of rows. Using this information positions are converted from the grid cells
to pixel coordinates. Routing 0 width edges between rows can also be interpreted as every second row and column being
reserved for edges. The row numbers in code are using first interpretation. To allow better centering of nodes one
above other each node is 2 columns wide and 1 row high.

# 1-2 Cycle removal and toposort

Cycle removal and toposort are done at the same time during single DFS traversal. In case entrypoint is part of a loop
DFS started from entrypoint. This ensures that entrypoint is at the top of resulting layout if possible. Resulting
toposort order is used in many of the following layout steps that require calculating some property of a vertice based
on child property or the other way around. Using toposort order such operations can be implemented iteration through
array in either forward or reverse direction. To prevent running out of stack memory when processing large graphs
DFS is implemented non-recursively.

# Layer assignment

Layers are assigned in toposort order from top to bottom, with nodes layer being max(predecessor.layer)+1. This ensures
that loop edges are only ones going from deeper levels to previous layers.

To further simply node placment a subset of edges is selected which forms a tree. This turns DAG drawing problem
into a tree drawing problem. For each node in level n following nodes which have level exactly n+1 are greedily
assigned as child nodes in tree. If a node already has perant assigned then corresponding edge is not part of tree.

# Node position assignment

Since the graph has been reduced to a tree node placement is more or less putting subtrees side by side with
parent on top. There is some room for interpretation what exactly side by side means and where exactly on top is.
Drawing the graph either too dense or too big may make it less readable so there are configuration options which allow
choosing these things resulting in more or less dense layout.

Current layout algorithm defines subtree size as it's bounding box and in most cases puts the bounding boxes side by
side. The layout could be made more dense by taking exact shape into account. There is a special case for ignoring
bounding box when one of 2 subtrees contain's exactly 1 vertice.

Other choice is wether to place node horizontally in the middle between direct child nodes or in the middle of
subtree width.

That results in 3 modes

- **wide** - bounding boxes are always side by side, no exception for single vertice subtree
- **medium** - use exception for single vertice subtree, place node in the middle of direct children. In case of long
 if elseif chanin produces staircase shape.
- **narrow** - use exception for single vertice subtree, place node in the middle of subtree total width. In case of
 if elseif chain produces two columns.

# Edge routing
Edge routing can be split into 3 stages. Rough routing within grid, overlaping edge prevention and converting to
pixel coordinates.

Due to nodes being placed in a grid. Horizontal segments of edges can't intersect with any nodes. The path for edges
is chosen so that it consists of at most 5 segments, typically resulting in sidedway U shape or square Z shape.
- short vertical segment from node to horizontal line
- move to empty column
- vertical segment between starting row and end row, an empty column can always be found, in the worst case there are empty columns at the sides of drawing
- horizontal segment to target node column
- short vertical segment connecting to target node

There are 3 special cases:
- source and target nodes are in the same column with no nodes betweeen - single vertical segment
- column bellow stating node is empty - segments 1-3 are merged
- column above target node is empty - segments 3-5 are merged
Vertical segment intersection with nodes is prevented using a 2d arry marking which vertical segments are blocked and
naively iterating through all rows between start and end at the desired column.

Edge overlap within a column or row is prevented by spliting columns into sub-columns. Used subcolumns are stored and
chechked using a 2d array of lists.

*/

namespace {
class MinTree1
{
public:
    MinTree1(size_t size)
        : size(size)
        , nodeCount(2 * size)
        , nodes(nodeCount)
    {
    }

    MinTree1(size_t size, int value)
        : MinTree1(size)
    {
        init(value);
    }

    void buildTree()
    {
        for (size_t i = size - 1; i > 0; i--) {
            nodes[i] = std::min(nodes[i << 1], nodes[(i << 1) | 1]);
        }
    }
    void init(int value)
    {
        std::fill_n(nodes.begin() + size, size, value);
        buildTree();
    }

    void set(size_t pos, int value)
    {
        pos = positionToLeaveIndex(pos);
        nodes[pos] = value;
        while (pos > 1) {
            auto parrent = pos >> 1;
            nodes[parrent] = std::min(nodes[pos], nodes[pos ^ 1]);
            pos = parrent;
        }
    }
    int valueAtPoint(size_t pos)
    {
        return nodes[positionToLeaveIndex(pos)];
    }
    size_t leaveIndexToPosition(size_t index)
    {
        return index - size;
    }

    size_t positionToLeaveIndex(size_t position)
    {
        return position + size;
    }

    /**
     * @brief Find right most position with value than less than given in range [0; position].
     * @param position inclusive right side of query range
     * @param value search for position with value less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int rightMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = positionToLeaveIndex(0), r = positionToLeaveIndex(position + 1); l < r;
                l >>= 1, r >>= 1) {
            if (l & 1) {
                if (isGood(l)) {
                    // mark subtree as good but don't stop yet, there might be something good further to the right
                    goodSubtree = l;
                }
                ++l;
            }
            if (r & 1) {
                --r;
                if (isGood(r)) {
                    goodSubtree = r;
                    break;
                }
            }
        }
        if (!goodSubtree) {
            return -1;
        }
        // find rightmost good leave
        while (goodSubtree < size) {
            goodSubtree = (goodSubtree << 1) + 1;
            if (!isGood(goodSubtree)) {
                goodSubtree ^= 1;
            }
        }
        return leaveIndexToPosition(goodSubtree); // convert from node index to position in range (0;size]
    }

    /**
     * @brief Find left most position with value than less than given in range [position; size).
     * @param position inclusive left side of query range
     * @param value search for position with value less than this
     * @return returns the position with searched property or -1 if there is no such position.
     */
    int leftMostLessThan(size_t position, int value)
    {
        auto isGood = [&](size_t pos) {
            return nodes[pos] < value;
        };
        // right side exclusive range [l;r)
        size_t goodSubtree = 0;
        for (size_t l = positionToLeaveIndex(position), r = positionToLeaveIndex(size); l < r;
                l >>= 1, r >>= 1) {
            if (l & 1) {
                if (isGood(l)) {
                    goodSubtree = l;
                    break;
                }
                ++l;
            }
            if (r & 1) {
                --r;
                if (isGood(r)) {
                    goodSubtree = r;
                    // mark subtree as good but don't stop yet, there might be something good further to the left
                }
            }
        }
        if (!goodSubtree) {
            return -1;
        }
        // find leftmost good leave
        while (goodSubtree < size) {
            goodSubtree = (goodSubtree << 1);
            if (!isGood(goodSubtree)) {
                goodSubtree ^= 1;
            }
        }
        return leaveIndexToPosition(goodSubtree); // convert from node index to position in range (0;size]
    }
private:
    const size_t size; //< number of leaves and also index of left most leave
    const size_t nodeCount;
    std::vector<int> nodes;
};


class LazySegmentTree
{
public:
    using NodeType = int;
    using PromiseType = int;
    static const int NO_PROMISE = INT_MIN;

    LazySegmentTree(size_t size)
        : size(size)
        , nodeCount(2 * size)
        , nodes(nodeCount)
        , promise(size, NO_PROMISE)
    {
        h = 0;
        size_t v = size;
        while (v) {
            v >>= 1;
            h++;
        }
    }

    LazySegmentTree(size_t size, NodeType value)
        : LazySegmentTree(size)
    {
        init(value);
    }

    size_t leaveIndexToPosition(size_t index) const
    {
        return index - size;
    }

    size_t positionToLeaveIndex(size_t position) const
    {
        return position + size;
    }

    void updateFromChild(NodeType &parent, const NodeType &left, const NodeType &right)
    {
        parent = std::max(left, right);
    }

    void pushDown(size_t parent)
    {
        if (promise[parent] != NO_PROMISE) {
            size_t left = (parent << 1);
            size_t right = (parent << 1) | 1;
            nodes[left] = nodes[right] = nodes[parent];
            if (left < size) {
                promise[left] = promise[parent];
            }
            if (right < size) {
                promise[right] = promise[parent];
            }
            promise[parent] = NO_PROMISE;
        }
    }

    void buildTree()
    {
        for (size_t i = size - 1; i > 0; i--) {
            updateFromChild(nodes[i], nodes[i << 1], nodes[(i << 1) | 1]);
        }
    }
    void init(NodeType value)
    {
        std::fill_n(nodes.begin() + size, size, value);
        buildTree();
        promise.assign(promise.size(), NO_PROMISE);
    }

    void pushDownFromRoot(size_t p)
    {
        for (size_t i = h; i > 0; i--) {
            pushDown(p >> i);
        }
    }

    void updateUntilRoot(size_t p)
    {
        while (p > 1) {
            auto parent = p >> 1;
            if (promise[parent] == NO_PROMISE) {
                updateFromChild(nodes[parent], nodes[p], nodes[p ^ 1]);
            }
            p >>= 1;
        }
    }

    void setRange(size_t left, size_t right, NodeType value)
    {
        left = positionToLeaveIndex(left);
        right = positionToLeaveIndex(right);
        pushDownFromRoot(left);
        pushDownFromRoot(right - 1);
        for (size_t l = left, r = right; l < r; l >>= 1, r >>= 1) {
            if (l & 1) {
                nodes[l] = value;
                if (l < size) {
                    promise[l] = value;
                }
                l += 1;
            }
            if (r & 1) {
                r -= 1;
                nodes[r] = value;
                if (r < size) {
                    promise[r] = value;
                }
            }
        }
        updateUntilRoot(left);
        updateUntilRoot(right - 1);
    }

    int rangeMaximum(size_t l, size_t r)
    {
        NodeType result = INT_MIN;
        l = positionToLeaveIndex(l);
        r = positionToLeaveIndex(r);
        pushDownFromRoot(l);
        pushDownFromRoot(r -1);
        for (; l < r; l >>= 1, r>>= 1) {
            if (l & 1) {
                result = std::max(result, nodes[l++]);
            }
            if (r & 1) {
                result = std::max(result, nodes[--r]);
            }
        }
        return result;
    }

private:
    const size_t size; //< number of leaves and also index of left most leave
    const size_t nodeCount;
    int h;
    std::vector<NodeType> nodes;
    std::vector<PromiseType> promise;
};
const int LazySegmentTree::NO_PROMISE;

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

    // assign layers and select tree edges
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
        for (size_t i = 0; i < blockIt.second.edges.size(); i++) {
            layoutState.edge[blockIt.first][i].dest = blockIt.second.edges[i].target;
        }
    }
    layoutState.columns = 1;
    layoutState.rows = 1;
    for (auto &node : layoutState.grid_blocks) {
        // count is at least index + 1
        layoutState.rows = std::max(layoutState.rows, size_t(node.second.row) + 1);
        // block is 2 column wide
        layoutState.columns = std::max(layoutState.columns, size_t(node.second.col) + 2);
    }
    layoutState.rowHeight.assign(layoutState.rows, 0);
    layoutState.columnWidth.assign(layoutState.columns, 0);
    for (auto &node : layoutState.grid_blocks) {
        const auto &inputBlock = blocks[node.first];
        layoutState.rowHeight[node.second.row] = std::max(inputBlock.height, layoutState.rowHeight[node.second.row]);
        layoutState.columnWidth[node.second.col] = std::max(inputBlock.width / 2, layoutState.columnWidth[node.second.col]);
        layoutState.columnWidth[node.second.col + 1] = std::max(inputBlock.width / 2, layoutState.columnWidth[node.second.col + 1]);
    }

    routeEdges(layoutState);

    convertToPixelCoordinates(layoutState, width, height);
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

void GraphGridLayout::routeEdges(GraphGridLayout::LayoutState &state) const
{
    calculateEdgeMainColumn(state);
    roughRouting(state);
    elaborateEdgePlacement(state);
}

void GraphGridLayout::calculateEdgeMainColumn(GraphGridLayout::LayoutState &state) const
{
    // find an empty column as close as possible to start or end
    // Use sweep line approach processing events sorted by row

    struct Event {
        size_t blockId;
        size_t edgeId;
        int row;
        enum Type {
            Edge = 0,
            Block = 1
        } type;
    };
    // create events
    std::vector<Event> events;
    events.reserve(state.grid_blocks.size() * 2);
    for (const auto &it : state.grid_blocks) {
        events.push_back({it.first, 0, it.second.row, Event::Block});
        const auto &inputBlock = (*state.blocks)[it.first];
        int startRow = it.second.row + 1;

        auto gridEdges = state.edge[it.first];
        gridEdges.resize(inputBlock.edges.size());
        for (size_t i = 0; i < inputBlock.edges.size(); i++) {
            auto targetId = inputBlock.edges[i].target;
            gridEdges[i].dest = targetId;
            const auto &targetGridBlock = state.grid_blocks[targetId];
            int endRow = targetGridBlock.row;
            Event e{it.first, i, std::max(startRow, endRow), Event::Edge};
            events.push_back(e);
        }
    }
    std::sort(events.begin(), events.end(),
        [](const Event & a, const Event & b) {
        if (a.row != b.row) {
            return a.row < b.row;
        }
        return static_cast<int>(a.type) < static_cast<int>(b.type);
    });

    // process events and choose main column for each edge
    MinTree1 blockedColumns(state.columns + 1, -1); // There are more columns than node columns
    for (const auto &event : events) {
        if (event.type == Event::Block) {
            auto block = state.grid_blocks[event.blockId];
            blockedColumns.set(block.col + 1, event.row);
        } else {
            auto block = state.grid_blocks[event.blockId];
            int column = block.col + 1;
            auto &edge = state.edge[event.blockId][event.edgeId];
            const auto &targetBlock = state.grid_blocks[edge.dest];
            auto topRow = std::min(block.row + 1, targetBlock.row);
            auto targetColumn = targetBlock.col + 1;

            // Prefer using the same column as starting node or target node.
            // It allows reducing amount of segments.
            if (blockedColumns.valueAtPoint(column) < topRow) {
                edge.mainColumn = column;
            } else if (blockedColumns.valueAtPoint(targetColumn) < topRow) {
                edge.mainColumn = targetColumn;
            } else {
                auto nearestLeft = blockedColumns.rightMostLessThan(column, topRow);
                auto nearestRight = blockedColumns.leftMostLessThan(column, topRow);
                // There should always be empty column at the sides of drawing
                assert(nearestLeft != -1 && nearestRight != -1);

                // choose closest column
                auto distanceLeft = column - nearestLeft + abs(targetColumn - nearestLeft);
                auto distanceRight = nearestRight - column + abs(targetColumn - nearestRight);
                if (distanceLeft != distanceRight) {
                    edge.mainColumn = distanceLeft < distanceRight ? nearestLeft : nearestRight;
                } else {
                    edge.mainColumn = event.edgeId < state.edge[event.blockId].size() / 2 ? nearestLeft : nearestRight;
                }
            }
        }
    }
}

void GraphGridLayout::roughRouting(GraphGridLayout::LayoutState &state) const
{
    for (auto &blockIt : state.grid_blocks) {
        auto &blockEdges = state.edge[blockIt.first];
        for (size_t i = 0; i < blockEdges.size(); i++) {
            auto &edge = blockEdges[i];
            const auto &start = blockIt.second;
            const auto &target = state.grid_blocks[edge.dest];

            edge.addPoint(start.row + 1, start.col + 1);
            if (edge.mainColumn != start.col + 1) {
                edge.addPoint(start.row + 1, start.col + 1, edge.mainColumn < start.col + 1 ? -1 : 1);
                edge.addPoint(start.row + 1, edge.mainColumn, target.row <= start.row ? -2 : 0);
            }
            int mainColumnKind = 0;
            if (edge.mainColumn < start.col + 1 && edge.mainColumn < target.col + 1) {
                mainColumnKind = +2;
            } else if (edge.mainColumn > start.col + 1 && edge.mainColumn > target.col + 1) {
                mainColumnKind = -2;
            } else if (edge.mainColumn == start.col + 1 && edge.mainColumn != target.col + 1) {
                mainColumnKind = edge.mainColumn < target.col + 1 ? 1 : -1;
            } else if (edge.mainColumn == target.col + 1 && edge.mainColumn != start.col + 1) {
                mainColumnKind = edge.mainColumn < start.col + 1 ? 1 : -1;
            }
            edge.addPoint(target.row, edge.mainColumn, mainColumnKind);
            if (target.col + 1 != edge.mainColumn) {
                edge.addPoint(target.row, target.col + 1, target.row <= start.row ? 2 : 0);
                edge.addPoint(target.row, target.col + 1, target.col + 1 < edge.mainColumn ? 1 : -1);
            }
        }
    }
}

namespace {
    struct EdgeSegment {
        int y0;
        int y1;
        int x;
        int kind;
        int edgeIndex;
    };
    struct NodeSide {
        int x;
        int y0;
        int y1;
        int size;
    };
}

void calculateSegmentOffsets(
        std::vector<EdgeSegment> &segments,
        std::vector<int> &edgeOffsets,
        std::vector<int> &edgeColumnWidth,
        std::vector<NodeSide> &nodeRightSide,
        std::vector<NodeSide> &nodeLeftSide,
        const std::vector<int> &columnWidth,
        size_t H,
        int spacing)
{
    for (auto &segment : segments) {
        if (segment.y0 > segment.y1) {
            std::swap(segment.y0, segment.y1);
        }
    }

    std::sort(segments.begin(), segments.end(), [](const EdgeSegment &a, const EdgeSegment &b){
        if (a.x != b.x) return a.x < b.x;
        if (a.kind != b.kind) return a.kind < b.kind;
        auto aSize = a.y1 - a.y0;
        auto bSize = b.y1 - b.y0;
        if (a.kind != 1) {
            return aSize < bSize;
        } else {
            return aSize > bSize;
        }
        return false;
    });

    auto compareNode = [](const NodeSide &a, const NodeSide &b) {
        return a.x < b.x;
    };
    sort(nodeRightSide.begin(), nodeRightSide.end(), compareNode);
    sort(nodeLeftSide.begin(), nodeLeftSide.end(), compareNode);

    LazySegmentTree maxSegment(H, INT_MIN);
    auto nextSegmentIt = segments.begin();
    auto rightSideIt = nodeRightSide.begin();
    auto leftSideIt = nodeLeftSide.begin();
    while (nextSegmentIt != segments.end()) {
        int x = nextSegmentIt->x;

        int leftColumWidth = 0;
        if (x > 0) {
            leftColumWidth = columnWidth[x - 1];
        }
        maxSegment.setRange(0, H, -leftColumWidth);
        while (rightSideIt != nodeRightSide.end() && rightSideIt->x + 1 < x) {
            rightSideIt++;
        }
        while (rightSideIt != nodeRightSide.end() && rightSideIt->x + 1 == x) {
            maxSegment.setRange(rightSideIt->y0, rightSideIt->y1 + 1, rightSideIt->size - leftColumWidth);
            rightSideIt++;
        }

        while (nextSegmentIt != segments.end() && nextSegmentIt->x == x && nextSegmentIt->kind <= 1) {
            int y = maxSegment.rangeMaximum(nextSegmentIt->y0, nextSegmentIt->y1 + 1);
            if (nextSegmentIt->kind != -2) {
                y = std::max(y, 0);
            }
            y += spacing;
            maxSegment.setRange(nextSegmentIt->y0, nextSegmentIt->y1 + 1, y);
            edgeOffsets[nextSegmentIt->edgeIndex] = y;
            nextSegmentIt++;
        }

        auto firstRightSideSegment = nextSegmentIt;
        auto middleWidth = std::max(maxSegment.rangeMaximum(0, H), 0);

        int rightColumnWidth = 0;
        if (x < static_cast<int>(columnWidth.size())) {
            rightColumnWidth = columnWidth[x];
        }

        maxSegment.setRange(0, H, -rightColumnWidth);
        while (leftSideIt != nodeLeftSide.end() && leftSideIt->x < x) {
            leftSideIt++;
        }
        while (leftSideIt != nodeLeftSide.end() && leftSideIt->x == x) {
            maxSegment.setRange(leftSideIt->y0, leftSideIt->y1 + 1, leftSideIt->size - rightColumnWidth);
            leftSideIt++;
        }
        while (nextSegmentIt != segments.end() && nextSegmentIt->x == x) {
            int y = maxSegment.rangeMaximum(nextSegmentIt->y0, nextSegmentIt->y1 + 1);
            y += spacing;
            maxSegment.setRange(nextSegmentIt->y0, nextSegmentIt->y1 + 1, y);
            edgeOffsets[nextSegmentIt->edgeIndex] = y;
            nextSegmentIt++;
        }
        auto rightSideMiddle = std::max(maxSegment.rangeMaximum(0, H), 0);
        for (auto it = firstRightSideSegment; it != nextSegmentIt; ++it) {
            edgeOffsets[it->edgeIndex] = middleWidth + (rightSideMiddle - edgeOffsets[it->edgeIndex]) + spacing;
        }
        edgeColumnWidth[x] = middleWidth + rightSideMiddle + spacing;
    }
}


/**
 * @brief Center the segments to the middle of edge columns when possible.
 * @param segmentOffsets offsets relative to the left side edge column.
 * @param edgeColumnWidth widths of edge columns
 * @param segments either all horizontal or all vertical edge segments
 */
void centerEdges(
        std::vector<int> &segmentOffsets,
        const std::vector<int> &edgeColumnWidth,
        const std::vector<EdgeSegment> &segments)
{
    /* Split segments in each edge column into non intersecting chunks. Center each chunk separately.
     *
     * Process segment endpoints sorted by x and y. Maintain count of currently started segments. When number of
     * active segments reaches 0 there is empty space between chunks.
     */
    struct Event {
        int x;
        int y;
        int index;
        bool start;
    };
    std::vector<Event> events;
    events.reserve(segments.size() * 2);
    for (const auto &segment : segments) {
        auto offset = segmentOffsets[segment.edgeIndex];
        // Exclude segments which are outside edge column and between the blocks. It's hard to ensure that moving
        // them doesn't cause overlap with blocks.
        if (offset >= 0 && offset <= edgeColumnWidth[segment.x]) {
            events.push_back({segment.x, segment.y0, segment.edgeIndex, true});
            events.push_back({segment.x, segment.y1, segment.edgeIndex, false});
        }
    }
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        // Process segment start events before end to ensure that activeSegmentCount doesn't go negative and only
        // reaches 0 at the end of chunk.
        return int(a.start) > int(b.start);
    });

    auto it = events.begin();
    while (it != events.end()) {
        auto chunkStart = it++;
        int activeSegmentCount = 1;
        int chunkWidth = 0;
        while (activeSegmentCount > 0) {
            activeSegmentCount += it->start ? 1 : -1;
            chunkWidth = std::max(chunkWidth, segmentOffsets[it->index]);
            it++;
        }
        int spacing = (edgeColumnWidth[chunkStart->x] - chunkWidth) / 2;
        for (auto segment = chunkStart; segment != it; segment++) {
            if (segment->start) {
                segmentOffsets[segment->index] += spacing;
            }
        }
    }
}

void GraphGridLayout::elaborateEdgePlacement(GraphGridLayout::LayoutState &state) const
{
    // Vertical segments
    std::vector<EdgeSegment> segments;
    std::vector<NodeSide> rightSides;
    std::vector<NodeSide> leftSides;
    std::vector<int> edgeOffsets;
    int edgeIndex = 0;
    for (auto &edgeListIt : state.edge) {
        for (const auto &edge : edgeListIt.second) {
            for (size_t j = 1; j < edge.points.size(); j += 2) {
                EdgeSegment segment;
                segment.y0 = edge.points[j - 1].row * 2; // edges in even rows
                segment.y1 = edge.points[j].row * 2;
                segment.x = edge.points[j].col;
                segment.kind = edge.points[j].kind;
                segment.edgeIndex = edgeIndex++;
                segments.push_back(segment);
            }
        }
    }
    for (auto &blockIt : state.grid_blocks) {
        auto &node = blockIt.second;
        auto w = (*state.blocks)[blockIt.first].width / 2;
        int row = node.row * 2 + 1; // blocks in odd rows
        leftSides.push_back({node.col, row, row, w});
        rightSides.push_back({node.col + 1, row, row, w});
    }
    state.edgeColumnWidth.assign(state.columns + 1, 0);
    edgeOffsets.resize(edgeIndex);
    calculateSegmentOffsets(segments, edgeOffsets, state.edgeColumnWidth, rightSides, leftSides, state.columnWidth, 2 * state.rows + 1, layoutConfig.block_horizontal_margin);
    centerEdges(edgeOffsets, state.edgeColumnWidth, segments);
    edgeIndex = 0;
    for (auto &edgeListIt : state.edge) {
        for (auto &edge : edgeListIt.second) {
            for (size_t j = 1; j < edge.points.size(); j += 2) {
                edge.points[j].offset = edgeOffsets[edgeIndex++];
            }
        }
    }

    segments.clear();
    leftSides.clear();
    rightSides.clear();

    // Horizontal segments
    edgeIndex = 0;
    for (auto &edgeListIt : state.edge) {
        for (const auto &edge : edgeListIt.second) {
            for (size_t j = 2; j < edge.points.size(); j += 2) {
                EdgeSegment segment;
                segment.y0 = edge.points[j - 1].col * 2;
                segment.y1 = edge.points[j].col * 2;
                segment.x = edge.points[j].row;
                segment.kind = edge.points[j].kind;
                segment.edgeIndex = edgeIndex++;
                segments.push_back(segment);
            }
        }
    }
    edgeOffsets.resize(edgeIndex);
    for (auto &blockIt : state.grid_blocks) {
        auto &node = blockIt.second;
        int leftColumn = node.col * 2 + 1;
        leftSides.push_back({node.row, leftColumn, leftColumn + 2, state.rowHeight[node.row]});
        auto h = (*state.blocks)[blockIt.first].height;
        rightSides.push_back({node.row, leftColumn, leftColumn + 2, h});
    }
    state.edgeRowHeight.assign(state.rows + 1, 0);
    edgeOffsets.resize(edgeIndex);
    calculateSegmentOffsets(segments, edgeOffsets, state.edgeRowHeight, rightSides, leftSides, state.rowHeight, 2 * state.columns + 1, layoutConfig.block_horizontal_margin);
    edgeIndex = 0;
    for (auto &edgeListIt : state.edge) {
        for (auto &edge : edgeListIt.second) {
            for (size_t j = 2; j < edge.points.size(); j += 2) {
                edge.points[j].offset = edgeOffsets[edgeIndex++];
            }
        }
    }
}

void GraphGridLayout::convertToPixelCoordinates(GraphGridLayout::LayoutState &state, int &width, int &height) const
{
    // calculate row and column offsets
    std::vector<int> edgeColumnOffset(state.edgeColumnWidth.size());
    std::vector<int> edgeRowOffset(state.edgeRowHeight.size());
    auto widthToPosition = [](int &position, int &width) {
        int currentPostion = position;
        position += width;
        width = currentPostion;
    };
    int xPosition = 0;
    for (size_t i = 0; i < state.columns; i++) {
        state.edgeColumnWidth[i] = std::max(layoutConfig.block_horizontal_margin, state.edgeColumnWidth[i]);
        edgeColumnOffset[i]  = state.edgeColumnWidth[i];
        widthToPosition(xPosition, edgeColumnOffset[i]);
        widthToPosition(xPosition, state.columnWidth[i]);
    }
    edgeColumnOffset[state.columns]  = state.edgeColumnWidth[state.columns];
    widthToPosition(xPosition, edgeColumnOffset[state.columns]);
    int yPosition = 0;
    for (size_t i = 0; i < state.rows; i++) {
        state.edgeRowHeight[i] = std::max(layoutConfig.block_horizontal_margin, state.edgeRowHeight[i]);
        edgeRowOffset[i] = state.edgeRowHeight[i];
        widthToPosition(yPosition, edgeRowOffset[i]);
        widthToPosition(yPosition, state.rowHeight[i]);
    }
    edgeRowOffset[state.rows] = state.edgeRowHeight[state.rows];
    widthToPosition(yPosition, edgeRowOffset[state.rows]);

    width = xPosition;
    height = yPosition;

    // pixel positions
    for (auto &block : (*state.blocks)) {
        const auto &gridBlock = state.grid_blocks[block.first];

        block.second.x = edgeColumnOffset[gridBlock.col + 1] + state.edgeColumnWidth[gridBlock.col + 1] / 2 -
                block.second.width / 2;
        block.second.y = state.rowHeight[gridBlock.row];
    }
    for (auto &it: (*state.blocks)) {
        auto &block = it.second;
        for (size_t i = 0; i < block.edges.size(); i++) {
            auto &resultEdge = block.edges[i];
            const auto &target = (*state.blocks)[resultEdge.target];
            resultEdge.polyline.push_back(QPointF(0, block.y + block.height));

            const auto &edge = state.edge[it.first][i];
            for (size_t j = 1; j < edge.points.size(); j++) {
                if (j & 1) { // vertical segment
                    int column = edge.points[j].col;
                    int x = edgeColumnOffset[column] + /*state.edgeColumnWidth[column]/2;*/ + edge.points[j].offset;
                    resultEdge.polyline.back().setX(x);
                    resultEdge.polyline.push_back(QPointF(x, 0));
                } else { // horizontal segment
                    int row = edge.points[j].row;
                    int y = edgeRowOffset[row] + /*state.edgeRowHeight[row]/2;// +*/ edge.points[j].offset;
                    resultEdge.polyline.back().setY(y);
                    resultEdge.polyline.push_back(QPointF(0, y));
                }
            }
            resultEdge.polyline.back().setY(target.y);
        }
    }
}

