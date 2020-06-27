#include "CutterGraphView.h"

#include "core/Cutter.h"
#include "common/Configuration.h"

#include <cmath>

static const int KEY_ZOOM_IN = Qt::Key_Plus + Qt::ControlModifier;
static const int KEY_ZOOM_OUT = Qt::Key_Minus + Qt::ControlModifier;
static const int KEY_ZOOM_RESET = Qt::Key_Equal + Qt::ControlModifier;

CutterGraphView::CutterGraphView(QWidget *parent)
    : GraphView(parent)
    , mFontMetrics(nullptr)
{
    connect(Core(), &CutterCore::graphOptionsChanged, this, &CutterGraphView::refreshView);
    connect(Config(), &Configuration::colorsUpdated, this, &CutterGraphView::colorsUpdatedSlot);
    connect(Config(), &Configuration::fontsUpdated, this, &CutterGraphView::fontsUpdatedSlot);

    initFont();
    colorsUpdatedSlot();
}

QPoint CutterGraphView::getTextOffset(int line) const
{
    int padding = static_cast<int>(2 * charWidth);
    return QPoint(padding, padding + line * charHeight);
}

void CutterGraphView::initFont()
{
    setFont(Config()->getFont());
    QFontMetricsF metrics(font());
    baseline = int(metrics.ascent());
    charWidth = metrics.width('X');
    charHeight = static_cast<int>(metrics.height());
    charOffset = 0;
    mFontMetrics.reset(new CachedFontMetrics<qreal>(font()));
}

void CutterGraphView::zoom(QPointF mouseRelativePos, double velocity)
{
    qreal newScale = getViewScale() * std::pow(1.25, velocity);
    setZoom(mouseRelativePos, newScale);
}

void CutterGraphView::setZoom(QPointF mouseRelativePos, double scale)
{
    mouseRelativePos.rx() *= size().width();
    mouseRelativePos.ry() *= size().height();
    mouseRelativePos /= getViewScale();

    auto globalMouse = mouseRelativePos + getViewOffset();
    mouseRelativePos *= getViewScale();
    qreal newScale = scale;
    newScale = std::max(newScale, 0.05);
    mouseRelativePos /= newScale;
    setViewScale(newScale);

    // Adjusting offset, so that zooming will be approaching to the cursor.
    setViewOffset(globalMouse.toPoint() - mouseRelativePos.toPoint());

    viewport()->update();
    emit viewZoomed();
}

void CutterGraphView::zoomIn()
{
    zoom(QPointF(0.5, 0.5), 1);
}

void CutterGraphView::zoomOut()
{
    zoom(QPointF(0.5, 0.5), -1);
}

void CutterGraphView::zoomReset()
{
    setZoom(QPointF(0.5, 0.5), 1);
}

void CutterGraphView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.alt_background");
    disassemblySelectedBackgroundColor = ConfigColor("gui.disass_selected");
    mDisabledBreakpointColor = disassemblyBackgroundColor;
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    disassemblySelectionColor = ConfigColor("lineHighlight");
    PCSelectionColor = ConfigColor("highlightPC");

    jmpColor = ConfigColor("graph.trufae");
    brtrueColor = ConfigColor("graph.true");
    brfalseColor = ConfigColor("graph.false");

    mCommentColor = ConfigColor("comment");
    initFont();
    refreshView();
}

GraphLayout::LayoutConfig CutterGraphView::getLayoutConfig()
{
    auto blockSpacing = Config()->getGraphBlockSpacing();
    auto edgeSpacing = Config()->getGraphEdgeSpacing();
    GraphLayout::LayoutConfig layoutConfig;
    layoutConfig.blockHorizontalSpacing = blockSpacing.x();
    layoutConfig.blockVerticalSpacing = blockSpacing.y();
    layoutConfig.edgeHorizontalSpacing = edgeSpacing.x();
    layoutConfig.edgeVerticalSpacing = edgeSpacing.y();
    return layoutConfig;
}

void CutterGraphView::fontsUpdatedSlot()
{
    initFont();
    refreshView();
}

bool CutterGraphView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key() + keyEvent->modifiers();
        if (key == KEY_ZOOM_OUT || key == KEY_ZOOM_RESET
                || key == KEY_ZOOM_IN || (key == (KEY_ZOOM_IN | Qt::ShiftModifier))) {
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key() + keyEvent->modifiers();
        if (key == KEY_ZOOM_IN || (key == (KEY_ZOOM_IN | Qt::ShiftModifier))) {
            zoomIn();
            return true;
        } else if (key == KEY_ZOOM_OUT) {
            zoomOut();
            return true;
        } else if (key == KEY_ZOOM_RESET) {
            zoomReset();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return GraphView::event(event);
}

void CutterGraphView::refreshView()
{
    initFont();
    setLayoutConfig(getLayoutConfig());
}

void CutterGraphView::wheelEvent(QWheelEvent *event)
{
    // when CTRL is pressed, we zoom in/out with mouse wheel
    if (Qt::ControlModifier == event->modifiers()) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            int numSteps = numDegrees.y() / 15;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            QPointF relativeMousePos = event->pos();
#else
            QPointF relativeMousePos = event->position();
#endif
            relativeMousePos.rx() /= size().width();
            relativeMousePos.ry() /= size().height();

            zoom(relativeMousePos, numSteps);
        }
        event->accept();
    } else {
        // use mouse wheel for scrolling when CTRL is not pressed
        GraphView::wheelEvent(event);
    }
    emit graphMoved();
}

void CutterGraphView::resizeEvent(QResizeEvent *event)
{
    GraphView::resizeEvent(event);
    emit resized();
}


void CutterGraphView::mousePressEvent(QMouseEvent *event)
{
    GraphView::mousePressEvent(event);
    emit graphMoved();
}

void CutterGraphView::mouseMoveEvent(QMouseEvent *event)
{
    GraphView::mouseMoveEvent(event);
    emit graphMoved();
}
