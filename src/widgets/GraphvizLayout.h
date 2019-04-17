#ifndef GRAPHVIZLAYOUT_H
#define GRAPHVIZLAYOUT_H

#include "core/Cutter.h"
#include "GraphLayout.h"

class GraphvizLayout : public GraphLayout
{
public:

    GraphvizLayout();
    virtual void CalculateLayout(std::unordered_map<ut64, GraphBlock> &blocks,
                                 ut64 entry,
                                 int &width,
                                 int &height) const override;
private:
};

#endif // GRAPHVIZLAYOUT_H
