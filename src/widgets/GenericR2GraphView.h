#ifndef GENERIC_R2_GRAPHVIEW_H
#define GENERIC_R2_GRAPHVIEW_H

#include "widgets/SimpleTextGraphView.h"

class GenericR2GraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    using SimpleTextGraphView::SimpleTextGraphView;
protected:
    void loadCurrentGraph() override;
};

#endif //GENERIC_R2_GRAPHVIEW_H
