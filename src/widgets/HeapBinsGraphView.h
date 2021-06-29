//
// Created by Pulak Malhotra on 29/06/21.
//

#ifndef CUTTER_HEAPBINSGRAPHVIEW_H
#define CUTTER_HEAPBINSGRAPHVIEW_H
#include "CutterGraphView.h"

class HeapBinsGraphView : public CutterGraphView
{
    Q_OBJECT
public:
    explicit HeapBinsGraphView(QWidget *parent, RzHeapBin *bin);
    void drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive) override;
    void loadGraph();
    void refreshView() override;

private:
    RzHeapBin *heapBin;
};

#endif // CUTTER_HEAPBINSGRAPHVIEW_H
