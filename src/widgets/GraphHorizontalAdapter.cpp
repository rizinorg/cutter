#include "GraphHorizontalAdapter.h"

GraphHorizontalAdapter::GraphHorizontalAdapter(std::unique_ptr<GraphLayout> layout)
    : GraphLayout({})
    , layout(std::move(layout))
{
    swapLayoutConfigDirection();
}

void GraphHorizontalAdapter::CalculateLayout(
    GraphLayout::Graph &blocks,
    unsigned long long entry,
    int &width,
    int &height) const
{
    for (auto &block : blocks) {
        std::swap(block.second.width, block.second.height);
    }
    layout->CalculateLayout(blocks, entry, height, width); // intentionally swapping height and width
    for (auto &block : blocks) {
        std::swap(block.second.width, block.second.height);
        std::swap(block.second.x, block.second.y);
        for (auto &edge : block.second.edges) {
            for (auto &point : edge.polyline) {
                std::swap(point.rx(), point.ry());
            }
            switch (edge.arrow) {
            case GraphEdge::Down:
                edge.arrow = GraphEdge::Right;
                break;
            case GraphEdge::Left:
                edge.arrow = GraphEdge::Up;
                break;
            case GraphEdge::Up:
                edge.arrow = GraphEdge::Left;
                break;
            case GraphEdge::Right:
                edge.arrow = GraphEdge::Down;
                break;
            case GraphEdge::None:
                edge.arrow = GraphEdge::None;
                break;
            }
        }
    }
}

void GraphHorizontalAdapter::setLayoutConfig(const GraphLayout::LayoutConfig &config)
{
    GraphLayout::setLayoutConfig(config);
    swapLayoutConfigDirection();
    layout->setLayoutConfig(config);
}

void GraphHorizontalAdapter::swapLayoutConfigDirection()
{
    std::swap(layoutConfig.edgeVerticalSpacing, layoutConfig.edgeHorizontalSpacing);
    std::swap(layoutConfig.blockVerticalSpacing, layoutConfig.blockHorizontalSpacing);
}
