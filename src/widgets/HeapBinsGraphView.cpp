#include <Configuration.h>
#include <dialogs/GlibcHeapInfoDialog.h>
#include "HeapBinsGraphView.h"

HeapBinsGraphView::HeapBinsGraphView(QWidget *parent, RzHeapBin *bin, MainWindow *main)
    : SimpleTextGraphView(parent, main), heapBin(bin)
{
    chunkInfoAction = new QAction(tr("Detailed Chunk Info"), this);
    addressableItemContextMenu.addAction(chunkInfoAction);
    addAction(chunkInfoAction);

    connect(chunkInfoAction, &QAction::triggered, this, &HeapBinsGraphView::viewChunkInfo);

    bits = Core()->getArchBits();

    enableAddresses(true);
}

void HeapBinsGraphView::viewChunkInfo()
{
    GlibcHeapInfoDialog heapInfoDialog(selectedBlock, QString(), this);
    heapInfoDialog.exec();
}

void HeapBinsGraphView::loadCurrentGraph()
{
    blockContent.clear();
    blocks.clear();

    RzListIter *iter;
    RzHeapChunkListItem *item;
    QVector<GraphHeapChunk> chunks;

    // if the bin is a fastbin or not
    bool singleLinkedBin = QString(heapBin->type) == QString("Fast")
            || QString(heapBin->type) == QString("Tcache");

    // store info about the chunks in a vector for easy access
    CutterRListForeach(heapBin->chunks, iter, RzHeapChunkListItem, item)
    {
        GraphHeapChunk graphHeapChunk;
        graphHeapChunk.addr = item->addr;
        RzHeapChunkSimple *chunkInfo = Core()->getHeapChunk(item->addr);
        if (!chunkInfo) {
            break;
        }
        QString content = "Chunk @ " + RAddressString(chunkInfo->addr) + "\nSize: "
                + RHexString(chunkInfo->size) + "\nFd: " + RAddressString(chunkInfo->fd);

        // fastbins lack bk pointer
        if (!singleLinkedBin) {
            content += "\nBk: " + RAddressString(chunkInfo->bk);
        }
        graphHeapChunk.fd = chunkInfo->fd;
        graphHeapChunk.bk = chunkInfo->bk;
        graphHeapChunk.content = content;
        chunks.append(graphHeapChunk);
        free(chunkInfo);
    }

    // fast and tcache bins have single linked list and other bins have double linked list
    if (singleLinkedBin) {
        display_single_linked_list(chunks);
    } else {
        display_double_linked_list(chunks);
    }

    cleanupEdges(blocks);
    computeGraphPlacement();
}

void HeapBinsGraphView::display_single_linked_list(QVector<GraphHeapChunk> chunks)
{
    bool tcache = QString(heapBin->type) == QString("Tcache");
    int ptrSize = bits;
    // add the graph block for the bin
    GraphLayout::GraphBlock gbBin;
    gbBin.entry = 1;
    gbBin.edges.emplace_back(heapBin->fd);
    QString content = tr(heapBin->type) + tr("bin ") + QString::number(heapBin->bin_num);
    if (tcache) {
        content += "\nEntry: " + RAddressString(heapBin->fd);
    } else {
        content += "\nFd: " + RAddressString(heapBin->fd);
    }
    addBlock(gbBin, content);

    // add the graph blocks for the chunks
    for (int i = 0; i < chunks.size(); i++) {
        GraphLayout::GraphBlock gbChunk;
        gbChunk.entry = chunks[i].addr;

        if (tcache && chunks[i].fd) {
            // base_address = address - 2 * PTR_SIZE
            gbChunk.edges.emplace_back(chunks[i].fd - 2 * ptrSize);
        } else {
            gbChunk.edges.emplace_back(chunks[i].fd);
        }

        if (i == chunks.size() - 1 && heapBin->message) {
            chunks[i].content += "\n" + QString(heapBin->message);
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
    // add the graph block for the bin
    GraphLayout::GraphBlock gbBin;
    gbBin.entry = heapBin->addr;
    gbBin.edges.emplace_back(heapBin->fd);
    gbBin.edges.emplace_back(heapBin->bk);
    QString content = tr(heapBin->type) + tr("bin ") + QString::number(heapBin->bin_num) + tr(" @ ")
            + RAddressString(heapBin->addr);
    content += "\nFd: " + RAddressString(heapBin->fd);
    content += "\nBk: " + RAddressString(heapBin->bk);

    addBlock(gbBin, content, heapBin->addr);

    // add the blocks for the chunks
    for (int i = 0; i < chunks.size(); i++) {
        GraphLayout::GraphBlock gbChunk;
        gbChunk.entry = chunks[i].addr;
        gbChunk.edges.emplace_back(chunks[i].fd);
        gbChunk.edges.emplace_back(chunks[i].bk);

        // if last chunk and there is message then show it in the chunk
        if (i == chunks.size() - 1 && heapBin->message) {
            chunks[i].content += "\n" + QString(heapBin->message);
        }

        addBlock(gbChunk, chunks[i].content, chunks[i].addr);
    }
}

// overriding this function from SimpleTextGraphView to support multiline text in graph block
// most code is shared from that implementation
void HeapBinsGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    // Render node
    auto &content = blockContent[block.entry];

    p.setPen(QColor(0, 0, 0, 0));
    p.setBrush(QColor(0, 0, 0, 100));
    p.setPen(QPen(graphNodeColor, 1));

    bool blockSelected = interactive && (block.entry == selectedBlock);
    if (blockSelected) {
        p.setBrush(disassemblySelectedBackgroundColor);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }
    // Draw basic block background
    p.drawRect(blockRect);

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    QRect screenChar = transform.mapRect(QRect(0, 0, ACharWidth, charHeight));

    if (screenChar.width() < Config()->getGraphMinFontSize()) {
        return;
    }

    p.setPen(palette().color(QPalette::WindowText));

    // Render node text
    // the only change from SimpleTextGraphView implementation
    p.drawText(blockRect, Qt::AlignCenter, content.text);
}

// overriding this function to support multiline text in graph blocks
void HeapBinsGraphView::addBlock(GraphLayout::GraphBlock block, const QString &text, RVA address)
{
    auto &content = blockContent[block.entry];
    content.text = text;
    content.address = address;

    int height = 1;
    double width = 0;

    // split text into different lines
    auto lines = text.split(QRegExp("[\n]"), QString::SkipEmptyParts);

    // width of the block is the maximum width of a line
    for (QString &line : lines) {
        width = std::max(mFontMetrics->width(line), width);
    }
    block.width = static_cast<int>(width + padding);
    block.height = (height * charHeight) * lines.length() + padding;
    GraphView::addBlock(std::move(block));
}

// overriding to support detailed heap info action in context menu
void HeapBinsGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                                  QContextMenuEvent *event, QPoint /*pos*/)
{
    if (haveAddresses) {
        const auto &content = blockContent[block.entry];
        selectedBlock = content.address;
        addressableItemContextMenu.setTarget(content.address, content.text);
        QPoint pos = event->globalPos();

        if (event->reason() != QContextMenuEvent::Mouse) {
            QPoint blockPosition(block.x + block.width / 2, block.y + block.height / 2);
            blockPosition = logicalToViewCoordinates(blockPosition);
            if (viewport()->rect().contains(blockPosition)) {
                pos = mapToGlobal(blockPosition);
            }
        }
        addressableItemContextMenu.exec(pos);
        event->accept();
    }
}