#ifndef CUTTER_HEAPBINSGRAPHVIEW_H
#define CUTTER_HEAPBINSGRAPHVIEW_H
#include "SimpleTextGraphView.h"

/**
 * @brief Dock widget providing a container to switch between different heap allocator views
 */
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
    void drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive) override;
    void addBlock(GraphLayout::GraphBlock block, const QString &text,
                  RVA address = RVA_INVALID) override;
    void blockContextMenuRequested(GraphView::GraphBlock &block, QContextMenuEvent *event,
                                   QPoint) override;

private slots:
    void viewChunkInfo();

private:
    RzHeapBin *heapBin;
    void displaySingleLinkedList(QVector<GraphHeapChunk>);
    void displayDoubleLinkedList(QVector<GraphHeapChunk>);
    QAction *chunkInfoAction;
    RVA selectedBlock;
    int bits;
};

#endif // CUTTER_HEAPBINSGRAPHVIEW_H
