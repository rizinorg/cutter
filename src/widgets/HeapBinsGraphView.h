//
// Created by Pulak Malhotra on 29/06/21.
//

#ifndef CUTTER_HEAPBINSGRAPHVIEW_H
#define CUTTER_HEAPBINSGRAPHVIEW_H
#include "SimpleTextGraphView.h"

class HeapBinsGraphView : public SimpleTextGraphView
{
    Q_OBJECT
    struct GraphHeapChunk
    {
        QString content;
        ut64 addr;
        ut64 fd;
        ut64 bk;
    };

public:
    explicit HeapBinsGraphView(QWidget *parent, RzHeapBin *bin, MainWindow *main);

protected:
    void loadCurrentGraph() override;

private:
    RzHeapBin *heapBin;
    void display_single_linked_list(QVector<GraphHeapChunk>);
    void display_double_linked_list(QVector<GraphHeapChunk>);
};

#endif // CUTTER_HEAPBINSGRAPHVIEW_H
