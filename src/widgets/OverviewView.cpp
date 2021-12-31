#include "OverviewView.h"
#include <QPainter>
#include <QMouseEvent>

#include "core/Cutter.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/TempConfig.h"

OverviewView::OverviewView(QWidget *parent) : GraphView(parent)
{
    connect(Config(), &Configuration::colorsUpdated, this, &OverviewView::colorsUpdatedSlot);
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

void OverviewView::centreRect()
{
    qreal w = rangeRect.width();
    qreal h = rangeRect.height();
    initialDiff = QPointF(w / 2, h / 2);
}

OverviewView::~OverviewView() {}

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

void OverviewView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    Q_UNUSED(interactive)
    QRectF blockRect(block.x, block.y, block.width, block.height);

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.drawRect(blockRect);
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawRect(blockRect.translated(2, 2));

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
    p.drawRect(blockRect);
}

void OverviewView::paintEvent(QPaintEvent *event)
{
    GraphView::paintEvent(event);
    if (rangeRect.width() == 0 && rangeRect.height() == 0) {
        return;
    }
    QPainter p(viewport());
    p.setPen(graphSelectionBorder);
    p.setBrush(graphSelectionFill);
    p.drawRect(rangeRect);
}

void OverviewView::mousePressEvent(QMouseEvent *event)
{
    mouseActive = true;
    auto pos = qhelpers::mouseEventPos(event);
    if (rangeRect.contains(pos)) {
        initialDiff = pos - rangeRect.topLeft();
    } else {
        QPointF size(rangeRect.width(), rangeRect.height());
        initialDiff = size * 0.5;
        rangeRect.moveCenter(pos);
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
    QPointF topLeft = qhelpers::mouseEventPos(event) - initialDiff;
    rangeRect.setTopLeft(topLeft);
    viewport()->update();
    emit mouseMoved();
}

void OverviewView::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

GraphView::EdgeConfiguration OverviewView::edgeConfiguration(GraphView::GraphBlock &from,
                                                             GraphView::GraphBlock *to,
                                                             bool interactive)
{
    Q_UNUSED(interactive)
    EdgeConfiguration ec;
    auto baseEcIt = edgeConfigurations.find({ from.entry, to->entry });
    if (baseEcIt != edgeConfigurations.end())
        ec = baseEcIt->second;
    ec.width_scale = 1.0 / getViewScale();
    return ec;
}

void OverviewView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.overview.node");
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    graphSelectionFill = ConfigColor("gui.overview.fill");
    graphSelectionBorder = ConfigColor("gui.overview.border");
    setCacheDirty();
    refreshView();
}

void OverviewView::setRangeRect(QRectF rect)
{
    rangeRect = rect;
    viewport()->update();
}
