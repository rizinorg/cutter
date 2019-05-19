#include "GraphView.h"

#include "GraphGridLayout.h"

#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPropertyAnimation>

#ifndef QT_NO_OPENGL
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QOpenGLPaintDevice>
#include <QOpenGLExtraFunctions>
#endif

GraphView::GraphView(QWidget *parent)
    : QAbstractScrollArea(parent)
    , graphLayoutSystem(new GraphGridLayout())
    , useGL(false)
#ifndef QT_NO_OPENGL
    , cacheTexture(0)
    , cacheFBO(0)
#endif
{
#ifndef QT_NO_OPENGL
    if (useGL) {
        glWidget = new QOpenGLWidget(this);
        setViewport(glWidget);
    } else {
        glWidget = nullptr;
    }
#endif
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

void GraphView::beginMouseDrag(QMouseEvent *event)
{
    scroll_base_x = event->x();
    scroll_base_y = event->y();
    scroll_mode = true;
    setCursor(Qt::ClosedHandCursor);
    viewport()->grabMouse();
}

void GraphView::setViewOffset(QPoint offset)
{
    setViewOffsetInternal(offset);
}

void GraphView::setViewScale(qreal scale)
{
    this->current_scale = scale;
    emit viewScaleChanged(scale);
}

QSize GraphView::getCacheSize()
{
    return
#ifndef QT_NO_OPENGL
        useGL ? cacheSize :
#endif
        pixmap.size();
}

qreal GraphView::getCacheDevicePixelRatioF()
{
    return
#ifndef QT_NO_OPENGL
        useGL ? 1.0 :
#endif
        pixmap.devicePixelRatioF();
}

QSize GraphView::getRequiredCacheSize()
{
    return
#ifndef QT_NO_OPENGL
        useGL ? viewport()->size() :
#endif
        viewport()->size() * devicePixelRatioF();
}

qreal GraphView::getRequiredCacheDevicePixelRatioF()
{
    return
#ifndef QT_NO_OPENGL
        useGL ? 1.0f :
#endif
        devicePixelRatioF();
}

void GraphView::paintEvent(QPaintEvent *)
{
#ifndef QT_NO_OPENGL
    if (useGL) {
        glWidget->makeCurrent();
    }
#endif

    if (!qFuzzyCompare(getCacheDevicePixelRatioF(), getRequiredCacheDevicePixelRatioF())
            || getCacheSize() != getRequiredCacheSize()) {
        setCacheDirty();
    }

    if (cacheDirty) {
        paintGraphCache();
        cacheDirty = false;
    }

    if (useGL) {
#ifndef QT_NO_OPENGL
        auto gl = glWidget->context()->extraFunctions();
        gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, cacheFBO);
        gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glWidget->defaultFramebufferObject());
        gl->glBlitFramebuffer(0, 0, cacheSize.width(), cacheSize.height(),
                              0, 0, viewport()->width(), viewport()->height(),
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glWidget->doneCurrent();
#endif
    } else {
        QRectF target(0.0, 0.0, viewport()->width(), viewport()->height());
        QRectF source(0.0, 0.0, pixmap.width(), pixmap.height());
        QPainter p(viewport());
        p.drawPixmap(target, pixmap, source);
    }
}

void GraphView::clampViewOffset()
{
    const qreal edgeFraction = 0.25;
    qreal edgeX = edgeFraction * (viewport()->width()  / current_scale);
    qreal edgeY = edgeFraction * (viewport()->height()  / current_scale);
    offset.rx() = std::max(std::min(qreal(offset.x()), width - edgeX),
                           - viewport()->width() / current_scale + edgeX);
    offset.ry() = std::max(std::min(qreal(offset.y()), height - edgeY),
                           - viewport()->height() / current_scale + edgeY);
}

void GraphView::setViewOffsetInternal(QPoint pos, bool emitSignal)
{
    offset = pos;
    clampViewOffset();
    if (emitSignal)
        emit viewOffsetChanged(offset);
}

void GraphView::addViewOffset(QPoint move, bool emitSignal)
{
    setViewOffsetInternal(offset + move, emitSignal);
}

void GraphView::paintGraphCache()
{
#ifndef QT_NO_OPENGL
    QOpenGLPaintDevice *paintDevice = nullptr;
#endif
    QPainter p;
    if (useGL) {
#ifndef QT_NO_OPENGL
        auto gl = QOpenGLContext::currentContext()->functions();

        bool resizeTex = false;
        if (!cacheTexture) {
            gl->glGenTextures(1, &cacheTexture);
            gl->glBindTexture(GL_TEXTURE_2D, cacheTexture);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            resizeTex = true;
        } else if (cacheSize != viewport()->size()) {
            gl->glBindTexture(GL_TEXTURE_2D, cacheTexture);
            resizeTex = true;
        }
        if (resizeTex) {
            cacheSize = viewport()->size();
            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, viewport()->width(), viewport()->height(), 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, nullptr);
            gl->glGenFramebuffers(1, &cacheFBO);
            gl->glBindFramebuffer(GL_FRAMEBUFFER, cacheFBO);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cacheTexture, 0);
        } else {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, cacheFBO);
        }
        gl->glViewport(0, 0, viewport()->width(), viewport()->height());
        gl->glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        gl->glClear(GL_COLOR_BUFFER_BIT);

        paintDevice = new QOpenGLPaintDevice(viewport()->size());
        p.begin(paintDevice);
#endif
    } else {
        auto dpr = devicePixelRatioF();
        pixmap = QPixmap(getRequiredCacheSize());
        pixmap.setDevicePixelRatio(dpr);
        p.begin(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
    }

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
            if (edge.polyline.empty()) {
                continue;
            }
            QPolygonF polyline = recalculatePolygon(edge.polyline);
            EdgeConfiguration ec = edgeConfiguration(block, &blocks[edge.target]);
            QPen pen(ec.color);
            pen.setWidth(pen.width() / ec.width_scale);
            if (pen.width() * current_scale < 2) {
                pen.setWidth(0);
            }
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

    p.end();
#ifndef QT_NO_OPENGL
    delete paintDevice;
#endif
}


void GraphView::center()
{
    centerX(false);
    centerY(false);
    emit viewOffsetChanged(offset);
}

void GraphView::centerX(bool emitSignal)
{
    offset.rx() = -((viewport()->width() - width * current_scale) / 2);
    offset.rx() /= current_scale;
    clampViewOffset();
    if (emitSignal) {
        emit viewOffsetChanged(offset);
    }
}

void GraphView::centerY(bool emitSignal)
{
    offset.ry() = -((viewport()->height() - height * current_scale) / 2);
    offset.ry() /= current_scale;
    clampViewOffset();
    if (emitSignal) {
        emit viewOffsetChanged(offset);
    }
}

void GraphView::showBlock(GraphBlock &block, bool anywhere)
{
    showBlock(&block, anywhere);
}

void GraphView::showBlock(GraphBlock *block, bool anywhere)
{
    showRectangle(QRect(block->x, block->y, block->width, block->height), anywhere);
    blockTransitionedTo(block);
}

void GraphView::showRectangle(const QRect &block, bool anywhere)
{
    QSizeF renderSize = QSizeF(viewport()->size()) / current_scale;
    if (width * current_scale <= viewport()->width()) {
        centerX(false);
    } else {
        if (!anywhere || block.x() < offset.x() || block.right() > offset.x() + renderSize.width()) {
            offset.rx() = block.x() - ((renderSize.width() - block.width()) / 2);
        }
    }
    if (height * current_scale <= viewport()->height()) {
        centerY(false);
    } else {
        static const int HEADER_HEIGHT = 35; // this could be handled better
        if (!anywhere || block.y() < offset.y() + HEADER_HEIGHT
                || block.bottom() > offset.y() + renderSize.height()) {
            offset.ry() = block.y() - HEADER_HEIGHT / current_scale;
        }
    }
    clampViewOffset();
    emit viewOffsetChanged(offset);
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
    if (event->button() == Qt::MiddleButton) {
        beginMouseDrag(event);
        return;
    }

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
    if (event->button() == Qt::LeftButton) {
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
    }

    // No block was clicked
    if (event->button() == Qt::LeftButton) {
        //Left click outside any block, enter scrolling mode
        beginMouseDrag(event);
        return;
    }

    QAbstractScrollArea::mousePressEvent(event);
}

void GraphView::mouseMoveEvent(QMouseEvent *event)
{
    if (scroll_mode) {
        addViewOffset(QPoint(scroll_base_x - event->x(), scroll_base_y - event->y()) / current_scale);
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

void GraphView::keyPressEvent(QKeyEvent *event)
{
    const int delta = static_cast<int>(30.0 / current_scale);
    int dx = 0, dy = 0;
    switch (event->key()) {
    case Qt::Key_Up:
        dy = -delta;
        break;
    case Qt::Key_Down:
        dy = delta;
        break;
    case Qt::Key_Left:
        dx = -delta;
        break;
    case Qt::Key_Right:
        dx = delta;
        break;
    default:
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }
    addViewOffset(QPoint(dx, dy));
    viewport()->update();
    event->accept();
}

void GraphView::mouseReleaseEvent(QMouseEvent *event)
{
    // TODO
//    if(event->button() == Qt::ForwardButton)
//        gotoNextSlot();
//    else if(event->button() == Qt::BackButton)
//        gotoPreviousSlot();

    if (scroll_mode && (event->buttons() & (Qt::LeftButton | Qt::MiddleButton)) == 0) {
        scroll_mode = false;
        setCursor(Qt::ArrowCursor);
        viewport()->releaseMouse();
    }
}

void GraphView::wheelEvent(QWheelEvent *event)
{
    if (scroll_mode) {
        // With some mice it's easy to hit sideway scroll button while holding middle mouse.
        // That would result in unwanted scrolling while panning.
        return;
    }
    QPoint delta = -event->angleDelta();
    delta /= current_scale;
    addViewOffset(delta);
    viewport()->update();
    event->accept();
}
