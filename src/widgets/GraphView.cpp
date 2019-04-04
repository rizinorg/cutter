#include "GraphView.h"

#include "GraphGridLayout.h"

#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>

GraphView::GraphView(QWidget *parent)
    : QAbstractScrollArea(parent)
    , graphLayoutSystem(new GraphGridLayout())
{
}

GraphView::~GraphView()
{
    // TODO: Cleanups
}

// Callbacks
void GraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block)
{
    Q_UNUSED(p);
    Q_UNUSED(block);
    qWarning() << "Draw block not overriden!";
}

void GraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
    qWarning() << "Block clicked not overridden!";
}

void GraphView::blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
    qWarning() << "Block double clicked not overridden!";
}

void GraphView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
}

bool GraphView::helpEvent(QHelpEvent *event)
{
    int x = event->pos().x() + offset.x();
    int y = event->pos().y() - offset.y();

    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        if ((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height)) {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockHelpEvent(block, event, pos);
            return true;
        }
    }

    return false;
}

void GraphView::blockTransitionedTo(GraphView::GraphBlock *to)
{
    Q_UNUSED(to);
    qWarning() << "blockTransitionedTo not overridden!";
}

GraphView::EdgeConfiguration GraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                          GraphView::GraphBlock *to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    qWarning() << "Edge configuration not overridden!";
    EdgeConfiguration ec;
    return ec;
}

bool GraphView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (helpEvent(static_cast<QHelpEvent *>(event))) {
            return true;
        }
    }

    return QAbstractScrollArea::event(event);
}

// This calculates the full graph starting at block entry.
void GraphView::computeGraph(ut64 entry)
{
    graphLayoutSystem->CalculateLayout(blocks, entry, width, height);
    ready = true;

    viewport()->update();
}

QPolygonF GraphView::recalculatePolygon(QPolygonF polygon)
{
    QPolygonF ret;
    for (int i = 0; i < polygon.size(); i++) {
        ret << QPointF(polygon[i].x() - offset.x(), polygon[i].y() - offset.y());
    }
    return ret;
}

void GraphView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    qreal dpr = devicePixelRatioF();
    if (useCache && qFuzzyCompare(dpr, pixmap.devicePixelRatioF())) {
        drawGraph();
        return;
    }
    pixmap = QPixmap(int(viewport()->width() * dpr), int(viewport()->height() * dpr));
    pixmap.setDevicePixelRatio(dpr);
    QPainter p(&pixmap);

    p.setRenderHint(QPainter::Antialiasing);

    int render_width = viewport()->width();
    int render_height = viewport()->height();

    p.setBrush(backgroundColor);
    p.drawRect(viewport()->rect());
    p.setBrush(Qt::black);

    p.scale(current_scale, current_scale);

    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        qreal blockX = block.x * current_scale;
        qreal blockY = block.y * current_scale;
        qreal blockWidth = block.width * current_scale;
        qreal blockHeight = block.height * current_scale;

        // Check if block is visible by checking if block intersects with view area
        if (offset.x() * current_scale < blockX + blockWidth
                && blockX < offset.x() * current_scale + render_width
                && offset.y() * current_scale < blockY + blockHeight
                && blockY < offset.y() * current_scale + render_height) {
            drawBlock(p, block);
        }

        p.setBrush(Qt::gray);

        // Always draw edges
        // TODO: Only draw edges if they are actually visible ...
        // Draw edges
        for (GraphEdge &edge : block.edges) {
            QPolygonF polyline = recalculatePolygon(edge.polyline);
            EdgeConfiguration ec = edgeConfiguration(block, &blocks[edge.target]);
            QPen pen(ec.color);
            pen.setWidth(pen.width() / ec.width_scale);
            p.setPen(pen);
            p.setBrush(ec.color);
            p.drawPolyline(polyline);
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);
            if (!polyline.empty()) {
                if (ec.start_arrow) {
                    auto firstPt = edge.polyline.first();
                    QPolygonF arrowStart;
                    arrowStart << QPointF(firstPt.x() - 3, firstPt.y() + 6);
                    arrowStart << QPointF(firstPt.x() + 3, firstPt.y() + 6);
                    arrowStart << QPointF(firstPt);
                    p.drawConvexPolygon(recalculatePolygon(arrowStart));
                }
                if (ec.end_arrow) {
                    auto lastPt = edge.polyline.last();
                    QPolygonF arrowEnd;
                    arrowEnd << QPointF(lastPt.x() - 3, lastPt.y() - 6);
                    arrowEnd << QPointF(lastPt.x() + 3, lastPt.y() - 6);
                    arrowEnd << QPointF(lastPt);
                    p.drawConvexPolygon(recalculatePolygon(arrowEnd));
                }
            }
        }
    }
    drawGraph();
    emit refreshBlock();
}

void GraphView::drawGraph()
{
    QRectF target(0.0, 0.0, viewport()->width(), viewport()->height());
    QRectF source(0.0, 0.0, viewport()->width() * pixmap.devicePixelRatioF(),
                  viewport()->height() * pixmap.devicePixelRatioF());
    QPainter p(viewport());
    p.drawPixmap(target, pixmap, source);
}


void GraphView::center()
{
    centerX();
    centerY();
}

void GraphView::centerX()
{
    offset.rx() = -((viewport()->width() - width * current_scale) / 2);
    offset.rx() /= current_scale;
}

void GraphView::centerY()
{
    offset.ry() = -((viewport()->height() - height * current_scale) / 2);
    offset.ry() /= current_scale;
}

void GraphView::showBlock(GraphBlock &block)
{
    showBlock(&block);
}

void GraphView::showBlock(GraphBlock *block)
{
    if (width * current_scale <= viewport()->width()) {
        centerX();
    } else {
        int render_width = viewport()->width() / current_scale;
        offset.rx() = block->x - ((render_width - block->width) / 2);
    }
    if (height * current_scale <= viewport()->height()) {
        centerY();
    } else {
        offset.ry() = block->y - 30;
    }
    blockTransitionedTo(block);
    viewport()->update();
}

void GraphView::addBlock(GraphView::GraphBlock block)
{
    blocks[block.entry] = block;
}

void GraphView::setEntry(ut64 e)
{
    entry = e;
}

bool GraphView::checkPointClicked(QPointF &point, int x, int y, bool above_y)
{
    int half_target_size = 5;
    if ((point.x() - half_target_size < x) &&
            (point.y() - (above_y ? (2 * half_target_size) : 0) < y) &&
            (x < point.x() + half_target_size) &&
            (y < point.y() + (above_y ? 0 : (3 * half_target_size)))) {
        return true;
    }
    return false;
}

// Mouse events
void GraphView::mousePressEvent(QMouseEvent *event)
{
    int x = event->pos().x() / current_scale + offset.x();
    int y = event->pos().y() / current_scale + offset.y();

    // Check if a block was clicked
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        if ((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height)) {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockClicked(block, event, pos);
            // Don't do anything else here! blockClicked might seek and
            // all our data is invalid then.
            return;
        }
    }

    // Check if a line beginning/end  was clicked
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;
        for (GraphEdge &edge : block.edges) {
            if (edge.polyline.length() < 2) {
                continue;
            }
            QPointF start = edge.polyline.first();
            QPointF end = edge.polyline.last();
            if (checkPointClicked(start, x, y)) {
                showBlock(blocks[edge.target]);
                // TODO: Callback to child
                return;
                break;
            }
            if (checkPointClicked(end, x, y, true)) {
                showBlock(block);
                // TODO: Callback to child
                return;
                break;
            }
        }
    }

    // No block was clicked
    if (event->button() == Qt::LeftButton) {
        //Left click outside any block, enter scrolling mode
        scroll_base_x = event->x();
        scroll_base_y = event->y();
        scroll_mode = true;
        setCursor(Qt::ClosedHandCursor);
        viewport()->grabMouse();
    }

}

void GraphView::mouseMoveEvent(QMouseEvent *event)
{
    if (scroll_mode) {
        offset.rx() += (scroll_base_x - event->x()) / current_scale;
        offset.ry() += (scroll_base_y - event->y()) / current_scale;
        scroll_base_x = event->x();
        scroll_base_y = event->y();
        viewport()->update();
    }
}

void GraphView::mouseDoubleClickEvent(QMouseEvent *event)
{
    int x = event->pos().x() / current_scale + offset.x();
    int y = event->pos().y() / current_scale + offset.y();

    // Check if a block was clicked
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        if ((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height)) {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockDoubleClicked(block, event, pos);
            return;
        }
    }
}

void GraphView::mouseReleaseEvent(QMouseEvent *event)
{
    // TODO
//    if(event->button() == Qt::ForwardButton)
//        gotoNextSlot();
//    else if(event->button() == Qt::BackButton)
//        gotoPreviousSlot();

    if (event->button() != Qt::LeftButton)
        return;

    if (scroll_mode) {
        scroll_mode = false;
        setCursor(Qt::ArrowCursor);
        viewport()->releaseMouse();
    }
}

void GraphView::wheelEvent(QWheelEvent *event)
{
    QPoint delta = -event->angleDelta();

    delta /= current_scale;
    offset += delta;

    viewport()->update();
    event->accept();
}
