//
// Created by Pulak Malhotra on 29/06/21.
//

#include <Configuration.h>
#include "HeapBinsGraphView.h"

HeapBinsGraphView::HeapBinsGraphView(QWidget *parent, RzHeapBin *bin, MainWindow *main)
    : SimpleTextGraphView(parent, main), heapBin(bin)
{
    enableAddresses(true);
}

void HeapBinsGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    RzListIter *iter;
    RzHeapChunkListItem *item;
    QVector<GraphHeapChunk> chunks;

    // store chunks in a vector from the list for easy access
    CutterRListForeach(heapBin->chunks, iter, RzHeapChunkListItem, item)
    {
        GraphHeapChunk graphHeapChunk;
        graphHeapChunk.addr = item->addr;
        RzHeapChunkSimple *chunkInfo = Core()->getHeapChunk(item->addr);
        if (!chunkInfo) {
            break;
        }
        QString content = "Base: " + RAddressString(chunkInfo->addr)
                + " Size: " + RHexString(chunkInfo->size);
        graphHeapChunk.fd = chunkInfo->fd;
        graphHeapChunk.bk = chunkInfo->bk;
        graphHeapChunk.content = content;
        chunks.append(graphHeapChunk);
        free(chunkInfo);
    }

    bool fast = QString(heapBin->type) == QString("Fast");
    if (fast) {
        display_single_linked_list(chunks);
    } else {
        display_double_linked_list(chunks);
    }

    cleanupEdges(blocks);
    computeGraphPlacement();
}

void HeapBinsGraphView::display_single_linked_list(QVector<GraphHeapChunk> chunks)
{
    GraphLayout::GraphBlock gbBin;
    gbBin.entry = 1;
    gbBin.edges.emplace_back(heapBin->fd);
    QString content = tr(heapBin->type) + tr("bin ") + QString::number(heapBin->bin_num);
    addBlock(gbBin, content);

    // add the blocks for the chunks
    for (int i = 0; i < chunks.size(); i++) {
        GraphLayout::GraphBlock gbChunk;
        gbChunk.entry = chunks[i].addr;
        gbChunk.edges.emplace_back(chunks[i].fd);

        if (i == chunks.size() - 1 && heapBin->message) {
            chunks[i].content += " " + QString(heapBin->message);
        }

        addBlock(gbChunk, chunks[i].content, chunks[i].addr);
    }

    // add the END block if no message
    if (!heapBin->message) {
        GraphLayout::GraphBlock gbEnd;
        gbEnd.entry = 0;
        addBlock(gbEnd, "END", 0);
    }
}

void HeapBinsGraphView::display_double_linked_list(QVector<GraphHeapChunk> chunks)
{
    // add the block for the bin
    GraphLayout::GraphBlock gbBin;
    gbBin.entry = heapBin->addr;
    gbBin.edges.emplace_back(heapBin->fd);
    gbBin.edges.emplace_back(heapBin->bk);
    QString content = tr(heapBin->type) + tr("bin ") + QString::number(heapBin->bin_num) + tr(" @ ")
            + RAddressString(heapBin->addr);
    addBlock(gbBin, content, heapBin->addr);

    // add the blocks for the chunks
    for (int i = 0; i < chunks.size(); i++) {
        GraphLayout::GraphBlock gbChunk;
        gbChunk.entry = chunks[i].addr;
        gbChunk.edges.emplace_back(chunks[i].fd);
        gbChunk.edges.emplace_back(chunks[i].bk);

        // if last chunk and there is message then show it in the chunk
        if (i == chunks.size() - 1 && heapBin->message) {
            chunks[i].content += " " + QString(heapBin->message);
        }

        addBlock(gbChunk, chunks[i].content, chunks[i].addr);
    }
}
