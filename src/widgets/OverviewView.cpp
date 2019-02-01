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

void OverviewView::adjustScale()
{
    if (horizontalScrollBar()->maximum() > 0) {
        current_scale = (qreal)viewport()->width() / (qreal)(horizontalScrollBar()->maximum() + horizontalScrollBar()->pageStep());
    }
    if (verticalScrollBar()->maximum() > 0) {
        qreal h_scale = (qreal)viewport()->height() / (qreal)(verticalScrollBar()->maximum() + verticalScrollBar()->pageStep());
        if (current_scale > h_scale) {
            current_scale = h_scale;
        }
    }
    adjustSize(viewport()->size().width(), viewport()->size().height());
    viewport()->update();
}

void OverviewView::refreshView()
{
    current_scale = 1.0;
    viewport()->update();
    adjustSize(viewport()->size().width(), viewport()->size().height());
    adjustScale();
}

void OverviewView::drawBlock(QPainter &p, GraphView::GraphBlock &block)
{
    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.drawRect(block.x, block.y, block.width, block.height);
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawRect(block.x + 2, block.y + 2,
               block.width, block.height);
    p.setPen(QPen(graphNodeColor, 1));
    p.setBrush(disassemblyBackgroundColor);
    p.drawRect(block.x, block.y,
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
    qreal x = event->localPos().x() - w/2;
    qreal y = event->localPos().y() - h/2;
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
    qreal w = rangeRect.width();
    qreal h = rangeRect.height();
    qreal real_width = width * current_scale;
    qreal real_height = height * current_scale;
    qreal max_right = unscrolled_render_offset_x + real_width;
    qreal max_bottom = unscrolled_render_offset_y + real_height;
    qreal rect_right = x + w;
    qreal rect_bottom = y + h;
    if (rect_right >= max_right) {
        x = unscrolled_render_offset_x + real_width - w;
    }
    if (rect_bottom >= max_bottom) {
        y = unscrolled_render_offset_y + real_height - h;
    }
    if (x <= unscrolled_render_offset_x) {
        x = unscrolled_render_offset_x;
    }
    if (y <= unscrolled_render_offset_y) {
        y = unscrolled_render_offset_y;
    }
    rangeRect = QRectF(x, y, w, h);
    viewport()->update();
    emit mouseMoved();
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
