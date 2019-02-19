#include "OverviewView.h"
#include <QPainter>
#include <QMouseEvent>

#include "Cutter.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/TempConfig.h"

OverviewView::OverviewView(QWidget *parent)
    : GraphView(parent)
{
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    colorsUpdatedSlot();
}

void OverviewView::setData(int baseWidth, int baseHeight, std::unordered_map<ut64, GraphBlock> baseBlocks)
{
    width = baseWidth;
    height = baseHeight;
    blocks = baseBlocks;
    refreshView();
}

OverviewView::~OverviewView()
{
}

void OverviewView::refreshView()
{
    current_scale = (qreal)viewport()->width() / width;
    qreal h_scale = (qreal)viewport()->height() / height;
    if (current_scale > h_scale) {
        current_scale = h_scale;
    }
    center();
    viewport()->update();
}

void OverviewView::drawBlock(QPainter &p, GraphView::GraphBlock &block)
{
    int blockX = block.x - offset_x;
    int blockY = block.y - offset_y;

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.drawRect(blockX, blockY, block.width, block.height);
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawRect(blockX + 2, blockY + 2,
               block.width, block.height);
    p.setPen(QPen(graphNodeColor, 1));
    p.setBrush(disassemblyBackgroundColor);
    p.drawRect(blockX, blockY,
               block.width, block.height);
    // Draw basic block highlighting/tracing
    auto bb = Core()->getBBHighlighter()->getBasicBlock(block.entry);
    if (bb) {
        QColor color(bb->color);
        color.setAlphaF(0.5);
        p.setBrush(color);
        p.drawRect(block.x, block.y,
                   block.width, block.height);
    }
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

bool OverviewView::mouseContainsRect(QMouseEvent *event)
{
    if (rangeRect.contains(event->pos())) {
        mouseActive = true;
        initialDiff = QPointF(event->localPos().x() - rangeRect.x(), event->localPos().y() - rangeRect.y());
        return true;
    }
    return false;
}

void OverviewView::mousePressEvent(QMouseEvent *event)
{
    if (mouseContainsRect(event)) {
        return;
    }
    qreal w = rangeRect.width();
    qreal h = rangeRect.height();
    qreal x = event->localPos().x() - w / 2;
    qreal y = event->localPos().y() - h / 2;
    rangeRect = QRectF(x, y, w, h);
    viewport()->update();
    emit mouseMoved();
    mouseContainsRect(event);
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
    Q_UNUSED(from);
    Q_UNUSED(to);
    EdgeConfiguration ec;
    ec.width_scale = current_scale;
    return ec;
}

void OverviewView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.overview.node");
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    refreshView();
}
