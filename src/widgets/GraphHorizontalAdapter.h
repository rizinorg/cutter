#ifndef GRAPH_HORIZONTAL_ADAPTER_H
#define GRAPH_HORIZONTAL_ADAPTER_H

#include "core/Cutter.h"
#include "GraphLayout.h"

/**
 * @brief Adapter for converting vertical graph layout into horizontal one.
 */
class GraphHorizontalAdapter : public GraphLayout
{
public:
    GraphHorizontalAdapter(std::unique_ptr<GraphLayout> layout);
    virtual void CalculateLayout(GraphLayout::Graph &blocks,
                                 ut64 entry,
                                 int &width,
                                 int &height) const override;
    void setLayoutConfig(const LayoutConfig &config) override;
private:
    std::unique_ptr<GraphLayout> layout;
    void swapLayoutConfigDirection();
};

#endif // GRAPH_HORIZONTAL_ADAPTER_H
