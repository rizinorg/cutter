/**
 * @file test_GraphLayouts.cpp
 * @brief Graph layouts keep the drawing readable for every control-flow shape.
 */

#include "IsolatedSettings.h"

#include "widgets/GraphGridLayout.h"
#include "widgets/GraphHorizontalAdapter.h"
#include "widgets/GraphView.h"
#ifdef CUTTER_ENABLE_GRAPHVIZ
#    include "widgets/GraphvizLayout.h"
#endif

#include <QRectF>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>
#include <set>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Graph = GraphLayout::Graph;
using GraphBlock = GraphLayout::GraphBlock;
using GraphEdge = GraphLayout::GraphEdge;

constexpr ut64 blockStride = 0x10;

struct BlockSpec
{
    ut64 address;
    int width;
    int height;
    std::vector<ut64> targets;
};

struct GraphCase
{
    GraphCase(const char *name, ut64 entry) : name(name), entry(entry) {}

    GraphCase &add(ut64 address, std::vector<ut64> targets)
    {
        constexpr ut64 sizeVariants = 4;
        constexpr int baseBlockWidth = 80;
        constexpr int blockWidthStep = 40;
        constexpr int baseBlockHeight = 30;
        constexpr int blockHeightStep = 15;
        const int variant = static_cast<int>((address / blockStride) % sizeVariants);
        return addSized(address, baseBlockWidth + variant * blockWidthStep,
                        baseBlockHeight + variant * blockHeightStep, std::move(targets));
    }

    GraphCase &addSized(ut64 address, int width, int height, std::vector<ut64> targets)
    {
        blocks.push_back({ address, width, height, std::move(targets) });
        return *this;
    }

    const char *name;
    ut64 entry;
    std::vector<BlockSpec> blocks;
};

enum class LayoutKind : ut8 {
    GridNarrow,
    GridMedium,
    GridWide,
    HorizontalNarrow,
    HorizontalMedium,
    HorizontalWide,
#ifdef CUTTER_ENABLE_GRAPHVIZ
    GraphvizPolyline,
    GraphvizOrtho,
#endif
};

struct LayoutSpec
{
    const char *name;
    LayoutKind kind;
};

GraphCase singleBlock()
{
    GraphCase graph("single_block", 0x10);
    graph.add(0x10, {});
    return graph;
}

GraphCase linearChain()
{
    GraphCase graph("linear_chain", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, { 0x30 });
    graph.add(0x30, {});
    return graph;
}

GraphCase ifElseDiamond()
{
    GraphCase graph("if_else_diamond", 0x10);
    graph.add(0x10, { 0x20, 0x30 });
    graph.add(0x20, { 0x40 });
    graph.add(0x30, { 0x40 });
    graph.add(0x40, {});
    return graph;
}

GraphCase ifWithoutElse()
{
    GraphCase graph("if_without_else", 0x10);
    graph.add(0x10, { 0x20, 0x30 });
    graph.add(0x20, { 0x30 });
    graph.add(0x30, {});
    return graph;
}

GraphCase selfLoop()
{
    GraphCase graph("self_loop", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, { 0x20, 0x30 });
    graph.add(0x30, {});
    return graph;
}

GraphCase loopBackToEntry()
{
    GraphCase graph("loop_back_to_entry", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, { 0x10, 0x30 });
    graph.add(0x30, {});
    return graph;
}

GraphCase nestedLoops()
{
    GraphCase graph("nested_loops", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, { 0x30 });
    graph.add(0x30, { 0x40 });
    graph.add(0x40, { 0x30, 0x50 });
    graph.add(0x50, { 0x20, 0x60 });
    graph.add(0x60, {});
    return graph;
}

GraphCase twoBackEdgesIntoOneBlock()
{
    GraphCase graph("two_back_edges_into_one_block", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, { 0x30, 0x40 });
    graph.add(0x30, { 0x20 });
    graph.add(0x40, { 0x20, 0x50 });
    graph.add(0x50, {});
    return graph;
}

GraphCase irreducibleLoop()
{
    GraphCase graph("irreducible_loop", 0x10);
    graph.add(0x10, { 0x20, 0x30 });
    graph.add(0x20, { 0x30 });
    graph.add(0x30, { 0x20, 0x40 });
    graph.add(0x40, {});
    return graph;
}

GraphCase longBackEdge()
{
    constexpr ut64 longBackEdgeLength = 8;
    GraphCase graph("long_back_edge", 0x10);
    const ut64 first = 0x10;
    const ut64 last = first + (longBackEdgeLength - 1) * blockStride;
    for (ut64 address = first; address < last; address += blockStride) {
        graph.add(address, { address + blockStride });
    }
    graph.add(last, { first + blockStride, last + blockStride });
    graph.add(last + blockStride, {});
    return graph;
}

GraphCase switchFanOutAndMerge()
{
    GraphCase graph("switch_fan_out_and_merge", 0x10);
    std::vector<ut64> cases;
    constexpr ut64 switchCaseCount = 8;
    constexpr int switchBlockWidth = 240;
    constexpr int switchBlockHeight = 40;
    const ut64 head = 0x10;
    const ut64 firstCase = 0x100;
    const ut64 merge = 0x200;
    for (ut64 i = 0; i < switchCaseCount; ++i) {
        const ut64 address = firstCase + i * blockStride;
        cases.push_back(address);
        graph.add(address, { merge });
    }
    graph.addSized(head, switchBlockWidth, switchBlockHeight, cases);
    graph.addSized(merge, switchBlockWidth, switchBlockHeight, {});
    return graph;
}

GraphCase wideEntryBlock()
{
    constexpr int wideBlockWidth = 600;
    constexpr int narrowBlockSize = 40;
    GraphCase graph("wide_entry_block", 0x10);
    graph.addSized(0x10, wideBlockWidth, narrowBlockSize, { 0x20, 0x30 });
    graph.addSized(0x20, narrowBlockSize, narrowBlockSize, { 0x40 });
    graph.addSized(0x30, narrowBlockSize, narrowBlockSize, { 0x40 });
    graph.addSized(0x40, narrowBlockSize, narrowBlockSize, {});
    return graph;
}

GraphCase unreachableBlocks()
{
    GraphCase graph("unreachable_blocks", 0x10);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, {});
    graph.add(0x30, { 0x40 });
    graph.add(0x40, { 0x20 });
    return graph;
}

GraphCase entryNotInGraph()
{
    GraphCase graph("entry_not_in_graph", 0xdead);
    graph.add(0x10, { 0x20 });
    graph.add(0x20, {});
    return graph;
}

std::vector<GraphCase> makeGraphCases()
{
    return {
        singleBlock(),       linearChain(),     ifElseDiamond(),        ifWithoutElse(),
        selfLoop(),          loopBackToEntry(), nestedLoops(),          twoBackEdgesIntoOneBlock(),
        irreducibleLoop(),   longBackEdge(),    switchFanOutAndMerge(), wideEntryBlock(),
        unreachableBlocks(), entryNotInGraph()
    };
}

const std::vector<GraphCase> &graphCases()
{
    static const std::vector<GraphCase> cases = makeGraphCases();
    return cases;
}

const std::vector<LayoutSpec> &layouts()
{
    static const std::vector<LayoutSpec> all = {
        { "grid_narrow", LayoutKind::GridNarrow },
        { "grid_medium", LayoutKind::GridMedium },
        { "grid_wide", LayoutKind::GridWide },
        { "horizontal_narrow", LayoutKind::HorizontalNarrow },
        { "horizontal_medium", LayoutKind::HorizontalMedium },
        { "horizontal_wide", LayoutKind::HorizontalWide },
#ifdef CUTTER_ENABLE_GRAPHVIZ
        { "graphviz_polyline", LayoutKind::GraphvizPolyline },
        { "graphviz_ortho", LayoutKind::GraphvizOrtho },
#endif
    };
    return all;
}

std::unique_ptr<GraphLayout> makeGrid(GraphGridLayout::LayoutType type)
{
    return std::make_unique<GraphGridLayout>(type);
}

std::unique_ptr<GraphLayout> makeLayout(LayoutKind kind)
{
    switch (kind) {
    case LayoutKind::GridNarrow:
        return makeGrid(GraphGridLayout::LayoutType::Narrow);
    case LayoutKind::GridMedium:
        return makeGrid(GraphGridLayout::LayoutType::Medium);
    case LayoutKind::GridWide:
        return makeGrid(GraphGridLayout::LayoutType::Wide);
    case LayoutKind::HorizontalNarrow:
        return std::make_unique<GraphHorizontalAdapter>(
                makeGrid(GraphGridLayout::LayoutType::Narrow));
    case LayoutKind::HorizontalMedium:
        return std::make_unique<GraphHorizontalAdapter>(
                makeGrid(GraphGridLayout::LayoutType::Medium));
    case LayoutKind::HorizontalWide:
        return std::make_unique<GraphHorizontalAdapter>(
                makeGrid(GraphGridLayout::LayoutType::Wide));
#ifdef CUTTER_ENABLE_GRAPHVIZ
    case LayoutKind::GraphvizPolyline:
        return std::make_unique<GraphvizLayout>(GraphvizLayout::LayoutType::DotPolyline);
    case LayoutKind::GraphvizOrtho:
        return std::make_unique<GraphvizLayout>(GraphvizLayout::LayoutType::DotOrtho);
#endif
    }
    return nullptr;
}

bool isHorizontal(LayoutKind kind)
{
    return kind == LayoutKind::HorizontalNarrow || kind == LayoutKind::HorizontalMedium
            || kind == LayoutKind::HorizontalWide;
}

bool isGrid(LayoutKind kind)
{
    return kind == LayoutKind::GridNarrow || kind == LayoutKind::GridMedium
            || kind == LayoutKind::GridWide || isHorizontal(kind);
}

/**
 * @brief Builds the layout input from a graph case.
 */
Graph buildGraph(const GraphCase &graphCase)
{
    Graph graph;
    for (const BlockSpec &spec : graphCase.blocks) {
        GraphBlock block;
        block.entry = spec.address;
        block.width = spec.width;
        block.height = spec.height;
        for (const ut64 target : spec.targets) {
            block.edges.emplace_back(target);
        }
        graph[spec.address] = block;
    }
    return graph;
}

QString hex(ut64 address)
{
    return QStringLiteral("0x%1").arg(static_cast<qulonglong>(address), 0, 16);
}

QString edgeName(ut64 from, ut64 to)
{
    return hex(from) + QStringLiteral(" -> ") + hex(to);
}

QRectF rectOf(const GraphBlock &block)
{
    return QRectF(block.x, block.y, block.width, block.height);
}

/**
 * @brief Whether the segment from a to b goes through the inside of rect.
 */
bool segmentCrossesInterior(const QPointF &a, const QPointF &b, const QRectF &rect)
{
    constexpr qreal borderTolerance = 0.5;
    const QRectF inner =
            rect.adjusted(borderTolerance, borderTolerance, -borderTolerance, -borderTolerance);
    if (inner.isEmpty()) {
        return false;
    }
    if (a.x() == b.x()) {
        const qreal top = std::min(a.y(), b.y());
        const qreal bottom = std::max(a.y(), b.y());
        return a.x() > inner.left() && a.x() < inner.right() && bottom > inner.top()
                && top < inner.bottom();
    }
    const qreal left = std::min(a.x(), b.x());
    const qreal right = std::max(a.x(), b.x());
    return a.y() > inner.top() && a.y() < inner.bottom() && right > inner.left()
            && left < inner.right();
}

/**
 * @brief The edges that a depth-first search from the entry doesn't see as loops.
 */
std::set<std::pair<ut64, ut64>> forwardEdgesFromEntry(const GraphCase &graphCase)
{
    std::unordered_map<ut64, const BlockSpec *> specs;
    for (const BlockSpec &spec : graphCase.blocks) {
        specs[spec.address] = &spec;
    }
    std::set<std::pair<ut64, ut64>> forward;
    if (specs.find(graphCase.entry) == specs.end()) {
        return forward;
    }

    enum class State : ut8 { NotVisited, InStack, Visited };
    std::unordered_map<ut64, State> state;
    std::stack<std::pair<ut64, size_t>> stack;
    state[graphCase.entry] = State::InStack;
    stack.push({ graphCase.entry, 0 });
    while (!stack.empty()) {
        const ut64 current = stack.top().first;
        const size_t edgeIndex = stack.top().second;
        const std::vector<ut64> &targets = specs.at(current)->targets;
        if (edgeIndex == targets.size()) {
            state[current] = State::Visited;
            stack.pop();
            continue;
        }
        ++stack.top().second;
        const ut64 target = targets[edgeIndex];
        State &targetState = state[target];
        if (targetState == State::NotVisited) {
            targetState = State::InStack;
            stack.push({ target, 0 });
            forward.insert({ current, target });
        } else if (targetState == State::Visited) {
            forward.insert({ current, target });
        }
    }
    return forward;
}

/**
 * @brief Fails if the layout added, removed or resized blocks or edges.
 */
void checkStructureUnchanged(const GraphCase &graphCase, const Graph &graph)
{
    QCOMPARE(graph.size(), graphCase.blocks.size());
    for (const BlockSpec &spec : graphCase.blocks) {
        const auto it = graph.find(spec.address);
        QVERIFY2(it != graph.end(), qPrintable(hex(spec.address) + " disappeared"));
        const GraphBlock &block = it->second;
        QCOMPARE(block.width, spec.width);
        QCOMPARE(block.height, spec.height);
        QCOMPARE(block.edges.size(), spec.targets.size());
        for (size_t i = 0; i < spec.targets.size(); ++i) {
            QCOMPARE(block.edges[i].target, spec.targets[i]);
        }
    }
}

/**
 * @brief Fails if a block, or a grid edge point, is outside the reported size.
 */
void checkInsideBounds(const Graph &graph, int width, int height, LayoutKind kind)
{
    for (const auto &it : graph) {
        const GraphBlock &block = it.second;
        QVERIFY2(block.x >= 0 && block.y >= 0 && block.x + block.width <= width
                         && block.y + block.height <= height,
                 qPrintable(hex(it.first) + " is outside the reported size"));
        if (!isGrid(kind)) {
            continue;
        }
        for (const GraphEdge &edge : block.edges) {
            for (const QPointF &point : edge.polyline) {
                QVERIFY2(point.x() >= 0 && point.y() >= 0 && point.x() <= width
                                 && point.y() <= height,
                         qPrintable(edgeName(it.first, edge.target)
                                    + " is outside the reported size"));
            }
        }
    }
}

/**
 * @brief Fails if two blocks overlap.
 */
void checkNoOverlap(const Graph &graph)
{
    std::vector<std::pair<ut64, QRectF>> rects;
    for (const auto &it : graph) {
        rects.emplace_back(it.first, rectOf(it.second));
    }
    for (size_t i = 0; i < rects.size(); ++i) {
        for (size_t j = i + 1; j < rects.size(); ++j) {
            QVERIFY2(!rects[i].second.intersects(rects[j].second),
                     qPrintable(hex(rects[i].first) + " overlaps " + hex(rects[j].first)));
        }
    }
}

void checkEdgeEnds(ut64 from, const GraphBlock &source, const GraphEdge &edge,
                   const GraphBlock &target, bool horizontal)
{
    const QString name = edgeName(from, edge.target);
    const QPointF start = edge.polyline.first();
    const QPointF end = edge.polyline.last();
    if (horizontal) {
        QVERIFY2(edge.arrow == GraphEdge::Right, qPrintable(name + " arrow doesn't point right"));
        QVERIFY2(start.x() == source.x + source.width && start.y() >= source.y
                         && start.y() <= source.y + source.height,
                 qPrintable(name + " doesn't start on the source's right side"));
        QVERIFY2(end.x() == target.x && end.y() >= target.y && end.y() <= target.y + target.height,
                 qPrintable(name + " doesn't end on the target's left side"));
    } else {
        QVERIFY2(edge.arrow == GraphEdge::Down, qPrintable(name + " arrow doesn't point down"));
        QVERIFY2(start.y() == source.y + source.height && start.x() >= source.x
                         && start.x() <= source.x + source.width,
                 qPrintable(name + " doesn't start on the source's bottom"));
        QVERIFY2(end.y() == target.y && end.x() >= target.x && end.x() <= target.x + target.width,
                 qPrintable(name + " doesn't end on the target's top"));
    }
}

/**
 * @brief Fails if a grid edge has a diagonal segment or crosses a block.
 */
void checkEdgeRoute(ut64 from, const GraphEdge &edge, const Graph &graph)
{
    const QString name = edgeName(from, edge.target);
    for (int i = 1; i < edge.polyline.size(); ++i) {
        const QPointF &a = edge.polyline[i - 1];
        const QPointF &b = edge.polyline[i];
        QVERIFY2(a.x() == b.x() || a.y() == b.y(), qPrintable(name + " has a diagonal segment"));
        for (const auto &it : graph) {
            QVERIFY2(!segmentCrossesInterior(a, b, rectOf(it.second)),
                     qPrintable(name + " cuts through " + hex(it.first)));
        }
    }
}

/**
 * @brief Fails if an edge doesn't start at its source or end at its target.
 */
void checkEdges(const Graph &graph, LayoutKind kind)
{
    for (const auto &it : graph) {
        for (const GraphEdge &edge : it.second.edges) {
            QVERIFY2(edge.polyline.size() >= 2,
                     qPrintable(edgeName(it.first, edge.target) + " has no route"));
            if (!isGrid(kind)) {
                continue;
            }
            checkEdgeEnds(it.first, it.second, edge, graph.at(edge.target), isHorizontal(kind));
            if (QTest::currentTestFailed()) {
                return;
            }
            checkEdgeRoute(it.first, edge, graph);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }
}

/**
 * @brief Fails if a forward edge points against the flow.
 */
void checkForwardEdges(const GraphCase &graphCase, const Graph &graph, LayoutKind kind)
{
    if (!isGrid(kind)) {
        return;
    }
    for (const auto &edge : forwardEdgesFromEntry(graphCase)) {
        const GraphBlock &source = graph.at(edge.first);
        const GraphBlock &target = graph.at(edge.second);
        const bool after = isHorizontal(kind) ? target.x >= source.x + source.width
                                              : target.y >= source.y + source.height;
        QVERIFY2(after, qPrintable(edgeName(edge.first, edge.second) + " goes against the flow"));
    }
}

/**
 * @brief Adds a data row for every graph case in every layout.
 */
void addCaseLayoutRows()
{
    QTest::addColumn<int>("caseIndex");
    QTest::addColumn<int>("layoutIndex");
    for (size_t c = 0; c < graphCases().size(); ++c) {
        for (size_t l = 0; l < layouts().size(); ++l) {
            QTest::addRow("%s/%s", graphCases()[c].name, layouts()[l].name)
                    << static_cast<int>(c) << static_cast<int>(l);
        }
    }
}

/**
 * @brief Adds a data row for every layout.
 */
void addLayoutRows()
{
    QTest::addColumn<int>("layoutIndex");
    for (size_t l = 0; l < layouts().size(); ++l) {
        QTest::addRow("%s", layouts()[l].name) << static_cast<int>(l);
    }
}

} // namespace

class TestGraphLayouts : public QObject
{
    Q_OBJECT

private slots:
    void emptyGraphSizeDoesNotDependOnCaller_data();
    void emptyGraphSizeDoesNotDependOnCaller();
    void singleBlockHasEqualMargins_data();
    void singleBlockHasEqualMargins();
    void layoutKeepsDrawingRules_data();
    void layoutKeepsDrawingRules();
    void layoutIsDeterministic_data();
    void layoutIsDeterministic();
    void veryLongChainKeepsOrder();
    void cleanupEdgesKeepsFirstEdgeToEachBlock_data();
    void cleanupEdgesKeepsFirstEdgeToEachBlock();
};

void TestGraphLayouts::emptyGraphSizeDoesNotDependOnCaller_data()
{
    addLayoutRows();
}

/**
 * @brief An empty graph gets the same size whatever width and height the caller passed in.
 */
void TestGraphLayouts::emptyGraphSizeDoesNotDependOnCaller()
{
    constexpr int callerWidth = 777;
    constexpr int callerHeight = 555;
    QFETCH(const int, layoutIndex);
    const auto layout = makeLayout(layouts().at(layoutIndex).kind);

    Graph first;
    int firstWidth = 0;
    int firstHeight = 0;
    layout->calculateLayout(first, 0x10, firstWidth, firstHeight);

    Graph second;
    int secondWidth = callerWidth;
    int secondHeight = callerHeight;
    layout->calculateLayout(second, 0x10, secondWidth, secondHeight);

    QVERIFY(first.empty());
    QVERIFY(second.empty());
    QCOMPARE(secondWidth, firstWidth);
    QCOMPARE(secondHeight, firstHeight);
}

void TestGraphLayouts::singleBlockHasEqualMargins_data()
{
    addLayoutRows();
}

/**
 * @brief A single block has the same margin on opposite sides in grid layouts.
 */
void TestGraphLayouts::singleBlockHasEqualMargins()
{
    QFETCH(const int, layoutIndex);
    const LayoutKind kind = layouts().at(layoutIndex).kind;
    if (!isGrid(kind)) {
        QSKIP("Graphviz decides its own margins");
    }

    constexpr int singleBlockWidth = 120;
    constexpr int singleBlockHeight = 60;
    GraphCase single("single", 0x10);
    single.addSized(0x10, singleBlockWidth, singleBlockHeight, {});
    Graph graph = buildGraph(single);
    int width = 0;
    int height = 0;
    makeLayout(kind)->calculateLayout(graph, 0x10, width, height);

    const GraphBlock &block = graph.at(0x10);
    QVERIFY(block.x > 0);
    QVERIFY(block.y > 0);
    QCOMPARE(width - (block.x + block.width), block.x);
    QCOMPARE(height - (block.y + block.height), block.y);
}

void TestGraphLayouts::layoutKeepsDrawingRules_data()
{
    addCaseLayoutRows();
}

/**
 * @brief Every graph shape keeps the drawing rules in every layout.
 *
 * Coordinates depend on block sizes and tuning, so the rules are relations: blocks stay inside the
 * reported size and don't overlap, edges leave their source and reach their target, grid edges are
 * orthogonal and don't cut through blocks, and forward edges point down (right when horizontal).
 */
void TestGraphLayouts::layoutKeepsDrawingRules()
{
    QFETCH(const int, caseIndex);
    QFETCH(const int, layoutIndex);
    const GraphCase &graphCase = graphCases().at(caseIndex);
    const LayoutKind kind = layouts().at(layoutIndex).kind;

    Graph graph = buildGraph(graphCase);
    int width = 0;
    int height = 0;
    makeLayout(kind)->calculateLayout(graph, graphCase.entry, width, height);

    checkStructureUnchanged(graphCase, graph);
    if (QTest::currentTestFailed()) {
        return;
    }
    checkInsideBounds(graph, width, height, kind);
    if (QTest::currentTestFailed()) {
        return;
    }
    checkNoOverlap(graph);
    if (QTest::currentTestFailed()) {
        return;
    }
    checkEdges(graph, kind);
    if (QTest::currentTestFailed()) {
        return;
    }
    checkForwardEdges(graphCase, graph, kind);
}

void TestGraphLayouts::layoutIsDeterministic_data()
{
    addCaseLayoutRows();
}

/**
 * @brief Laying out the same graph twice gives the same positions and edges.
 */
void TestGraphLayouts::layoutIsDeterministic()
{
    QFETCH(const int, caseIndex);
    QFETCH(const int, layoutIndex);
    const GraphCase &graphCase = graphCases().at(caseIndex);
    const LayoutKind kind = layouts().at(layoutIndex).kind;

    Graph first = buildGraph(graphCase);
    Graph second = buildGraph(graphCase);
    int firstWidth = 0;
    int firstHeight = 0;
    int secondWidth = 0;
    int secondHeight = 0;
    makeLayout(kind)->calculateLayout(first, graphCase.entry, firstWidth, firstHeight);
    makeLayout(kind)->calculateLayout(second, graphCase.entry, secondWidth, secondHeight);

    QCOMPARE(secondWidth, firstWidth);
    QCOMPARE(secondHeight, firstHeight);
    for (const auto &it : first) {
        const GraphBlock &a = it.second;
        const GraphBlock &b = second.at(it.first);
        QCOMPARE(b.x, a.x);
        QCOMPARE(b.y, a.y);
        for (size_t i = 0; i < a.edges.size(); ++i) {
            QVERIFY(b.edges[i].polyline == a.edges[i].polyline);
            QVERIFY(b.edges[i].arrow == a.edges[i].arrow);
        }
    }
}

/**
 * @brief A 50000-block chain is laid out top to bottom in order.
 */
void TestGraphLayouts::veryLongChainKeepsOrder()
{
    constexpr ut64 veryLongChainLength = 50000;
    constexpr int chainBlockWidth = 100;
    constexpr int chainBlockHeight = 20;
    GraphCase chain("very_long_chain", 0);
    for (ut64 i = 0; i < veryLongChainLength; ++i) {
        std::vector<ut64> next;
        if (i + 1 < veryLongChainLength) {
            next.push_back(i + 1);
        }
        chain.addSized(i, chainBlockWidth, chainBlockHeight, next);
    }

    Graph graph = buildGraph(chain);
    int width = 0;
    int height = 0;
    GraphGridLayout(GraphGridLayout::LayoutType::Wide)
            .calculateLayout(graph, chain.entry, width, height);

    QCOMPARE(graph.size(), chain.blocks.size());
    for (ut64 i = 0; i + 1 < veryLongChainLength; ++i) {
        const GraphBlock &current = graph.at(i);
        QVERIFY2(graph.at(i + 1).y >= current.y + current.height,
                 qPrintable(edgeName(i, i + 1) + " goes against the flow"));
    }
}

void TestGraphLayouts::cleanupEdgesKeepsFirstEdgeToEachBlock_data()
{
    QTest::addColumn<QList<ut64>>("targets");
    QTest::addColumn<QList<ut64>>("expected");

    QTest::newRow("edge_leaving_graph") << QList<ut64> { 0x20, 0xdead } << QList<ut64> { 0x20 };
    QTest::newRow("only_edges_leaving_graph") << QList<ut64> { 0xdead, 0xbeef } << QList<ut64> {};
    QTest::newRow("duplicate_targets_keep_first_order")
            << QList<ut64> { 0x30, 0x20, 0x30, 0x20 } << QList<ut64> { 0x30, 0x20 };
    QTest::newRow("self_loop") << QList<ut64> { 0x10, 0x20 } << QList<ut64> { 0x10, 0x20 };
    QTest::newRow("no_edges") << QList<ut64> {} << QList<ut64> {};
}

/**
 * @brief cleanupEdges drops edges leaving the graph and duplicate edges, keeping the first one.
 */
void TestGraphLayouts::cleanupEdgesKeepsFirstEdgeToEachBlock()
{
    QFETCH(const QList<ut64>, targets);
    QFETCH(const QList<ut64>, expected);

    GraphCase cleanup("cleanup", 0x10);
    cleanup.add(0x10, std::vector<ut64>(targets.cbegin(), targets.cend()));
    cleanup.add(0x20, {});
    cleanup.add(0x30, {});
    Graph graph = buildGraph(cleanup);

    GraphView::cleanupEdges(graph);

    QCOMPARE(graph.size(), size_t(3));
    QList<ut64> kept;
    for (const GraphEdge &edge : graph.at(0x10).edges) {
        kept.append(edge.target);
    }
    QCOMPARE(kept, expected);
    QVERIFY(graph.at(0x20).edges.empty());
    QVERIFY(graph.at(0x30).edges.empty());

    GraphView::cleanupEdges(graph);
    QCOMPARE(graph.at(0x10).edges.size(), size_t(expected.size()));
}

CUTTER_UNIT_TEST_MAIN(TestGraphLayouts)
#include "test_GraphLayouts.moc"
