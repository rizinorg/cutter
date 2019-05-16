#include "OverviewView.h"
#include <QPainter>
#include <QMouseEvent>

#include "core/Cutter.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/TempConfig.h"

OverviewView::OverviewView(QWidget *parent)
    : GraphView(parent)
{
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    colorsUpdatedSlot();
}

void OverviewView::setData(int baseWidth, int baseHeight,
                           std::unordered_map<ut64, GraphBlock> baseBlocks,
                           DisassemblerGraphView::EdgeConfigurationMapping baseEdgeConfigurations)
{
    width = baseWidth;
    height = baseHeight;
    blocks = baseBlocks;
    edgeConfigurations = baseEdgeConfigurations;
    scaleAndCenter();
    setCacheDirty();
    viewport()->update();
}

OverviewView::~OverviewView()
{
}

void OverviewView::scaleAndCenter()
{
    qreal wScale = (qreal)viewport()->width() / width;
    qreal hScale = (qreal)viewport()->height() / height;
    setViewScale(std::min(wScale, hScale));
    center();
}

void OverviewView::refreshView()
{
    scaleAndCenter();
    viewport()->update();
}

void OverviewView::drawBlock(QPainter &p, GraphView::GraphBlock &block)
{
    int blockX = block.x - getViewOffset().x();
    int blockY = block.y - getViewOffset().y();

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.drawRect(blockX, blockY, block.width, block.height);
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawRect(blockX + 2, blockY + 2,
               block.width, block.height);

    // Draw basic block highlighting/tracing
    auto bb = Core()->getBBHighlighter()->getBasicBlock(block.entry);
    if (bb) {
        QColor color(bb->color);
        color.setAlphaF(0.5);
        p.setBrush(color);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }
    p.setPen(QPen(graphNodeColor, 1));
    p.drawRect(blockX, blockY,
               block.width, block.height);
}

void OverviewView::paintEvent(QPaintEvent *event)
{
    GraphView::paintEvent(event);
    if (rangeRect.width() == 0 && rangeRect.height() == 0) {
        return;
    }
    QPainter p(viewport());
    p.setPen(Qt::red);
    p.drawRect(rangeRect);
}

void OverviewView::mousePressEvent(QMouseEvent *event)
{
    mouseActive = true;
    if (rangeRect.contains(event->pos())) {
        initialDiff = QPointF(event->localPos().x() - rangeRect.x(), event->localPos().y() - rangeRect.y());
    } else {
        qreal w = rangeRect.width();
        qreal h = rangeRect.height();
        qreal x = event->localPos().x() - w / 2;
        qreal y = event->localPos().y() - h / 2;
        rangeRect = QRectF(x, y, w, h);
        initialDiff = QPointF(w / 2,  h / 2);
        viewport()->update();
        emit mouseMoved();
    }
}

void OverviewView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseActive = false;
    GraphView::mouseReleaseEvent(event);
}

void OverviewView::mouseMoveEvent(QMouseEvent *event)
{
    if (!mouseActive) {
        return;
    }
    qreal x = event->localPos().x() - initialDiff.x();
    qreal y = event->localPos().y() - initialDiff.y();
    rangeRect = QRectF(x, y, rangeRect.width(), rangeRect.height());
    viewport()->update();
    emit mouseMoved();
}

void OverviewView::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

GraphView::EdgeConfiguration OverviewView::edgeConfiguration(GraphView::GraphBlock &from,
                                                             GraphView::GraphBlock *to)
{
    EdgeConfiguration ec;
    auto baseEcIt = edgeConfigurations.find({from.entry, to->entry});
    if (baseEcIt != edgeConfigurations.end())
        ec = baseEcIt->second;
    ec.width_scale = getViewScale();
    return ec;
}

void OverviewView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.overview.node");
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    setCacheDirty();
    refreshView();
}

void OverviewView::setRangeRect(QRectF rect)
{
    rangeRect = rect;
    viewport()->update();
}
