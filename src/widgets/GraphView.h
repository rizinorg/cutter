#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QObject>
#include <QPainter>
#include <QWidget>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QHelpEvent>

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>

#include "core/Cutter.h"
#include "widgets/GraphLayout.h"

#ifndef QT_NO_OPENGL
class QOpenGLWidget;
#endif

class GraphView : public QAbstractScrollArea
{
    Q_OBJECT

signals:
    void viewOffsetChanged(QPoint offset);
    void viewScaleChanged(qreal scale);

public:
    using GraphBlock = GraphLayout::GraphBlock;
    using GraphEdge = GraphLayout::GraphEdge;

    struct EdgeConfiguration {
        QColor color = QColor(128, 128, 128);
        bool start_arrow = false;
        bool end_arrow = true;
        qreal width_scale = 1.0;
    };

    explicit GraphView(QWidget *parent);
    ~GraphView() override;

    void showBlock(GraphBlock &block, bool anywhere = false);
    void showBlock(GraphBlock *block, bool anywhere = false);
    /**
     * @brief Move view so that area is visible.
     * @param rect Rectangle to show
     * @param anywhere - set to true for minimizing movement
     */
    void showRectangle(const QRect &rect, bool anywhere = false);

    /**
     * @brief keep the current addr of the fcn of Graph
     * Everytime overview updates its contents, it compares this value with the one in Graph
     * if they aren't same, then Overview needs to update the pixmap cache.
     */
    ut64 currentFcnAddr = RVA_INVALID; // TODO: move application specific code out of graph view

protected:
    std::unordered_map<ut64, GraphBlock> blocks;
    QColor backgroundColor = QColor(Qt::white);

    // Padding inside the block
    int block_padding = 16;

    void setCacheDirty()    { cacheDirty = true; }

    void addBlock(GraphView::GraphBlock block);
    void setEntry(ut64 e);
    void computeGraph(ut64 entry);

    // Callbacks that should be overridden
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block);
    virtual void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos);
    virtual void blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos);
    virtual void blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos);
    virtual bool helpEvent(QHelpEvent *event);
    virtual void blockTransitionedTo(GraphView::GraphBlock *to);
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from, GraphView::GraphBlock *to);

    bool event(QEvent *event) override;

    // Mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    int width = 0;
    int height = 0;

    void clampViewOffset();
    void setViewOffsetInternal(QPoint pos, bool emitSignal = true);
    void addViewOffset(QPoint move, bool emitSignal = true);

private:
    void centerX(bool emitSignal);
    void centerY(bool emitSignal);

    void paintGraphCache();

    bool checkPointClicked(QPointF &point, int x, int y, bool above_y = false);

    // Zoom data
    qreal current_scale = 1.0;

    QPoint offset = QPoint(0, 0);

    ut64 entry;

    std::unique_ptr<GraphLayout> graphLayoutSystem;

    bool ready = false;

    // Scrolling data
    int scroll_base_x = 0;
    int scroll_base_y = 0;
    bool scroll_mode = false;

    // Todo: remove charheight/charwidth cause it should be handled in child class
    qreal charWidth = 10.0;

    bool useGL;

    /**
     * @brief pixmap that caches the graph nodes
     */
    QPixmap pixmap;

#ifndef QT_NO_OPENGL
    uint32_t cacheTexture;
    uint32_t cacheFBO;
    QSize cacheSize;
    QOpenGLWidget *glWidget;
#endif

    /**
     * @brief flag to control if the cache is invalid and should be re-created in the next draw
     */
    bool cacheDirty = true;
    QSize getCacheSize();
    qreal getCacheDevicePixelRatioF();
    QSize getRequiredCacheSize();
    qreal getRequiredCacheDevicePixelRatioF();

    QPolygonF recalculatePolygon(QPolygonF polygon);
    void beginMouseDrag(QMouseEvent *event);

public:
    QPoint getViewOffset() const    { return offset; }
    void setViewOffset(QPoint offset);
    qreal getViewScale() const      { return current_scale; }
    void setViewScale(qreal scale);

    void center();
    void centerX()  { centerX(true); }
    void centerY()  { centerY(true); }
};

#endif // GRAPHVIEW_H
