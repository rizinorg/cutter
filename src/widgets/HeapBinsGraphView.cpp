//
// Created by Pulak Malhotra on 29/06/21.
//

#include <Configuration.h>
#include "HeapBinsGraphView.h"

HeapBinsGraphView::HeapBinsGraphView(QWidget *parent, RzHeapBin *bin)
    : CutterGraphView(parent), heapBin(bin)
{
}

void HeapBinsGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    qDebug() << "Drawing a block\n";
    QRectF blockRect(block.x, block.y, block.width, block.height);
    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);
}

void HeapBinsGraphView::loadGraph()
{
    blocks.clear();
    RzListIter *iter;
    RzHeapChunkListItem *item;
    CutterRListForeach(heapBin->chunks, iter, RzHeapChunkListItem, item)
    {
        GraphBlock gb;
        gb.entry = item->addr;
        addBlock(gb);
    }
}

void HeapBinsGraphView::refreshView()
{
    qDebug() << "refreshing all";
    CutterGraphView::refreshView();
    loadGraph();
}
