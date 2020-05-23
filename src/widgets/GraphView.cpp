#include "GraphView.h"

#include "GraphGridLayout.h"
#ifdef CUTTER_ENABLE_GRAPHVIZ
#include "GraphvizLayout.h"
#endif
#include "Helpers.h"

#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPropertyAnimation>
#include <QSvgGenerator>

#ifndef CUTTER_NO_OPENGL_GRAPH
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QOpenGLPaintDevice>
#include <QOpenGLExtraFunctions>
#endif

GraphView::GraphView(QWidget *parent)
    : QAbstractScrollArea(parent)
    , useGL(false)
#ifndef CUTTER_NO_OPENGL_GRAPH
    , cacheTexture(0)
    , cacheFBO(0)
#endif
{
#ifndef CUTTER_NO_OPENGL_GRAPH
    if (useGL) {
        glWidget = new QOpenGLWidget(this);
        setViewport(glWidget);
    } else {
        glWidget = nullptr;
    }
#endif
    setGraphLayout(Layout::GridMedium);
}

GraphView::~GraphView()
{
}

// Callbacks
void GraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    Q_UNUSED(p)
    Q_UNUSED(block)
    Q_UNUSED(interactive)
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
    auto p = viewToLogicalCoordinates(event->pos());
    if (auto block = getBlockContaining(p)) {
        blockHelpEvent(*block, event, p - QPoint(block->x, block->y));
        return true;
    }
    return false;
}

void GraphView::blockTransitionedTo(GraphView::GraphBlock *to)
{
    Q_UNUSED(to);
    qWarning() << "blockTransitionedTo not overridden!";
}

GraphView::EdgeConfiguration GraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                          GraphView::GraphBlock *to,
                                                          bool interactive)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
    Q_UNUSED(interactive)
    qWarning() << "Edge configuration not overridden!";
    EdgeConfiguration ec;
    return ec;
}

void GraphView::blockContextMenuRequested(GraphView::GraphBlock &, QContextMenuEvent *, QPoint)
{
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

void GraphView::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
    if (event->reason() == QContextMenuEvent::Mouse) {
        QPoint p = viewToLogicalCoordinates(event->pos());
        if (auto block = getBlockContaining(p)) {
            blockContextMenuRequested(*block, event, p);
        }
    }
}

// This calculates the full graph starting at block entry.
void GraphView::computeGraph(ut64 entry)
{
    graphLayoutSystem->CalculateLayout(blocks, entry, width, height);
    ready = true;

    viewport()->update();
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
#ifndef CUTTER_NO_OPENGL_GRAPH
        useGL ? cacheSize :
#endif
        pixmap.size();
}

qreal GraphView::getCacheDevicePixelRatioF()
{
    return
#ifndef CUTTER_NO_OPENGL_GRAPH
        useGL ? 1.0 :
#endif
        qhelpers::devicePixelRatio(&pixmap);
}

QSize GraphView::getRequiredCacheSize()
{
    return viewport()->size() * qhelpers::devicePixelRatio(this);
}

qreal GraphView::getRequiredCacheDevicePixelRatioF()
{
    return
#ifndef CUTTER_NO_OPENGL_GRAPH
        useGL ? 1.0f :
#endif
        qhelpers::devicePixelRatio(this);
}

void GraphView::paintEvent(QPaintEvent *)
{
#ifndef CUTTER_NO_OPENGL_GRAPH
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
#ifndef CUTTER_NO_OPENGL_GRAPH
        auto gl = glWidget->context()->extraFunctions();
        gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, cacheFBO);
        gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glWidget->defaultFramebufferObject());
        auto dpr = qhelpers::devicePixelRatio(this);
        gl->glBlitFramebuffer(0, 0, cacheSize.width(), cacheSize.height(),
                              0, 0, viewport()->width() * dpr, viewport()->height() * dpr,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glWidget->doneCurrent();
#endif
    } else {
        QPainter p(viewport());
        p.drawPixmap(QPoint(0, 0), pixmap);
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
#ifndef CUTTER_NO_OPENGL_GRAPH
    std::unique_ptr<QOpenGLPaintDevice> paintDevice;
#endif
    QPainter p;
    if (useGL) {
#ifndef CUTTER_NO_OPENGL_GRAPH
        auto gl = QOpenGLContext::currentContext()->functions();

        bool resizeTex = false;
        QSize sizeNeed = getRequiredCacheSize();
        if (!cacheTexture) {
            gl->glGenTextures(1, &cacheTexture);
            gl->glBindTexture(GL_TEXTURE_2D, cacheTexture);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            resizeTex = true;
        } else if (cacheSize != sizeNeed) {
            gl->glBindTexture(GL_TEXTURE_2D, cacheTexture);
            resizeTex = true;
        }
        if (resizeTex) {
            cacheSize = sizeNeed;
            gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cacheSize.width(), cacheSize.height(), 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, nullptr);
            gl->glGenFramebuffers(1, &cacheFBO);
            gl->glBindFramebuffer(GL_FRAMEBUFFER, cacheFBO);
            gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cacheTexture, 0);
        } else {
            gl->glBindFramebuffer(GL_FRAMEBUFFER, cacheFBO);
        }
        gl->glViewport(0, 0, viewport()->width(), viewport()->height());
        gl->glClearColor(backgroundColor.redF(), backgroundColor.greenF(), backgroundColor.blueF(), 1.0f);
        gl->glClear(GL_COLOR_BUFFER_BIT);

        paintDevice.reset(new QOpenGLPaintDevice(cacheSize));
        p.begin(paintDevice.get());
#endif
    } else {
        auto dpr = qhelpers::devicePixelRatio(this);
        pixmap = QPixmap(getRequiredCacheSize() * dpr);
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill(backgroundColor);
        p.begin(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setViewport(this->viewport()->rect());
    }
    paint(p, offset, this->viewport()->rect(), current_scale);

    p.end();
}

void GraphView::paint(QPainter &p, QPoint offset, QRect viewport, qreal scale, bool interactive)
{
    QPointF offsetF(offset.x(), offset.y());
    p.setBrush(Qt::black);

    int render_width = viewport.width();
    int render_height = viewport.height();

    // window - rectangle in logical coordinates
    QRect window = QRect(offset, QSize(qRound(render_width / scale), qRound(render_height / scale)));
    p.setWindow(window);
    QRectF windowF(window.x(), window.y(), window.width(), window.height());

    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        QRectF blockRect(block.x, block.y, block.width, block.height);

        // Check if block is visible by checking if block intersects with view area
        if (blockRect.intersects(windowF)) {
            drawBlock(p, block, interactive);
        }

        p.setBrush(Qt::gray);

        // Always draw edges
        // TODO: Only draw edges if they are actually visible ...
        // Draw edges
        for (GraphEdge &edge : block.edges) {
            if (edge.polyline.empty()) {
                continue;
            }
            QPolygonF polyline = edge.polyline;
            EdgeConfiguration ec = edgeConfiguration(block, &blocks[edge.target], interactive);
            QPen pen(ec.color);
            pen.setStyle(ec.lineStyle);
            pen.setWidthF(pen.width() * ec.width_scale);
            if (scale_thickness_multiplier && ec.width_scale > 1.01 && pen.widthF() * scale < 2) {
                pen.setWidthF(ec.width_scale / scale);
            }
            if (pen.widthF() * scale < 2) {
                pen.setWidth(0);
            }
            p.setPen(pen);
            p.setBrush(ec.color);
            p.drawPolyline(polyline);
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);

            auto drawArrow = [&](QPointF tip, QPointF dir) {
                pen.setWidth(0);
                p.setPen(pen);
                QPolygonF arrow;
                arrow << tip;
                QPointF dy(-dir.y(), dir.x());
                QPointF base = tip - dir * 6;
                arrow << base + 3 * dy;
                arrow << base - 3 * dy;
                p.drawConvexPolygon(arrow);
            };

            if (!polyline.empty()) {
                if (ec.start_arrow) {
                    auto firstPt = edge.polyline.first();
                    drawArrow(firstPt, QPointF(0, 1));
                }
                if (ec.end_arrow) {
                    auto lastPt = edge.polyline.last();
                    QPointF dir(0, -1);
                    switch (edge.arrow) {
                    case GraphLayout::GraphEdge::Down:
                        dir = QPointF(0, 1);
                        break;
                    case GraphLayout::GraphEdge::Up:
                        dir = QPointF(0, -1);
                        break;
                    case GraphLayout::GraphEdge::Left:
                        dir = QPointF(-1, 0);
                        break;
                    case GraphLayout::GraphEdge::Right:
                        dir = QPointF(1, 0);
                        break;
                    default:
                        break;
                    }
                    drawArrow(lastPt, dir);
                }
            }
        }
    }
}

void GraphView::saveAsBitmap(QString path, const char *format, double scaler, bool transparent)
{
    QImage image(width*scaler, height*scaler, QImage::Format_ARGB32);
    if(transparent){
        image.fill(qRgba(0, 0, 0, 0));
    }else{
        image.fill(backgroundColor);
    }
    QPainter p;
    p.begin(&image);
    paint(p, QPoint(0, 0), image.rect(), scaler, false);
    p.end();
    if (!image.save(path, format)) {
        qWarning() << "Could not save image";
    }
}

void GraphView::saveAsSvg(QString path)
{
    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(QSize(width, height));
    generator.setViewBox(QRect(0, 0, width, height));
    generator.setTitle("Cutter graph export");
    QPainter p;
    p.begin(&generator);
    paint(p, QPoint(0, 0), QRect(0, 0, width, height), 1.0, false);
    p.end();
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
        if (!anywhere || block.y() < offset.y()
                || block.bottom() > offset.y() + renderSize.height()) {
            offset.ry() = block.y();
            // Leave some space at top if possible
            const qreal topPadding = 10 / current_scale;
            if (block.height() + topPadding < renderSize.height()) {
                offset.ry() -= topPadding;
            }
        }
    }
    clampViewOffset();
    emit viewOffsetChanged(offset);
    viewport()->update();
}

GraphView::GraphBlock *GraphView::getBlockContaining(QPoint p)
{
    // Check if a block was clicked
    for (auto &blockIt : blocks) {
        GraphBlock &block = blockIt.second;

        QRect rec(block.x, block.y, block.width, block.height);
        if (rec.contains(p)) {
            return &block;
        }
    }
    return nullptr;
}

QPoint GraphView::viewToLogicalCoordinates(QPoint p)
{
    return p / current_scale + offset;
}

void GraphView::setGraphLayout(GraphView::Layout layout)
{
    graphLayout = layout;
    switch (layout) {
    case Layout::GridNarrow:
        this->graphLayoutSystem.reset(new GraphGridLayout(GraphGridLayout::LayoutType::Narrow));
        break;
    case Layout::GridMedium:
        this->graphLayoutSystem.reset(new GraphGridLayout(GraphGridLayout::LayoutType::Medium));
        break;
    case Layout::GridWide:
        this->graphLayoutSystem.reset(new GraphGridLayout(GraphGridLayout::LayoutType::Wide));
        break;
#ifdef CUTTER_ENABLE_GRAPHVIZ
    case Layout::GraphvizOrtho:
        this->graphLayoutSystem.reset(new GraphvizLayout(GraphvizLayout::LineType::Ortho));
        break;
    case Layout::GraphvizOrthoLR:
        this->graphLayoutSystem.reset(new GraphvizLayout(GraphvizLayout::LineType::Ortho,
                                                         GraphvizLayout::Direction::LR));
        break;
    case Layout::GraphvizPolyline:
        this->graphLayoutSystem.reset(new GraphvizLayout(GraphvizLayout::LineType::Polyline));
        break;
    case Layout::GraphvizPolylineLR:
        this->graphLayoutSystem.reset(new GraphvizLayout(GraphvizLayout::LineType::Polyline,
                                                         GraphvizLayout::Direction::LR));
        break;
#endif
    }
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

    QPoint pos = viewToLogicalCoordinates(event->pos());

    // Check if a block was clicked
    if (auto block = getBlockContaining(pos)) {
        blockClicked(*block, event, pos - QPoint(block->x, block->y));
        // Don't do anything else here! blockClicked might seek and
        // all our data is invalid then.
        return;
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
                if (checkPointClicked(start, pos.x(), pos.y())) {
                    showBlock(blocks[edge.target]);
                    // TODO: Callback to child
                    return;
                    break;
                }
                if (checkPointClicked(end, pos.x(), pos.y(), true)) {
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
    auto p = viewToLogicalCoordinates(event->pos());
    if (auto block = getBlockContaining(p)) {
        blockDoubleClicked(*block, event, p - QPoint(block->x, block->y));
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
