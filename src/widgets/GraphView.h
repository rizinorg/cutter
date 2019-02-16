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

#include "Cutter.h"

class GraphView : public QAbstractScrollArea
{
    Q_OBJECT

    enum class LayoutType {
        Medium,
        Wide,
        Narrow,
    };

signals:
    void refreshBlock();

public:
    struct GraphBlock;

    struct Point {
        int row; //point[0]
        int col; //point[1]
        int index; //point[2]
    };

    struct GraphEdge {
        QColor color;
        GraphBlock *dest;
        std::vector<Point> points;
        int start_index = 0;

        QPolygonF polyline;
        QPolygonF arrow_start;
        QPolygonF arrow_end;

        void addPoint(int row, int col, int index = 0)
        {
            Point point = {row, col, 0};
            this->points.push_back(point);
            if (int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct GraphBlock {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        // This is a unique identifier, e.g. offset in the case of r2 blocks
        ut64 entry;
        // This contains unique identifiers to entries
        // Outgoing edges
        std::vector<ut64> exits;
        // Incoming edges
        std::vector<ut64> incoming;
        // TODO what is this
        std::vector<ut64> new_exits;

        // Number of rows in block
        int row_count = 0;
        // Number of columns in block
        int col_count = 0;
        // Column in which the block is
        int col = 0;
        // Row in which the block is
        int row = 0;

        // Edges
        std::vector<GraphEdge> edges;
    };

    struct EdgeConfiguration {
        QColor color = QColor(128, 128, 128);
        bool start_arrow = false;
        bool end_arrow = true;
        qreal width_scale = 1.0;
    };

    explicit GraphView(QWidget *parent);
    ~GraphView() override;
    void paintEvent(QPaintEvent *event) override;

    void showBlock(GraphBlock &block);
    void showBlock(GraphBlock *block);

    // Zoom data
    qreal current_scale = 1.0;

    int offset_x = 0;
    int offset_y = 0;

protected:
    std::unordered_map<ut64, GraphBlock> blocks;
    QColor backgroundColor = QColor(Qt::white);
    // The vertical margin between blocks
    int block_vertical_margin = 40;
    int block_horizontal_margin = 10;

    // Padding inside the block
    int block_padding = 16;


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

    void center();
    void centerX();
    void centerY();
    int width = 0;
    int height = 0;
private:
    bool checkPointClicked(QPointF &point, int x, int y, bool above_y = false);

    ut64 entry;

    void computeGraphLayout(GraphBlock &block);
    void adjustGraphLayout(GraphBlock &block, int col, int row);

    // Layout type
    LayoutType layoutType = LayoutType::Medium;

    bool ready = false;

    // Scrolling data
    int scroll_base_x = 0;
    int scroll_base_y = 0;
    bool scroll_mode = false;


    // Todo: remove charheight/charwidth cause it should be handled in child class
    qreal charWidth = 10.0;

    // Edge computing stuff
    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;
    std::vector<int> col_edge_x;
    std::vector<int> row_edge_y;
    bool isEdgeMarked(EdgesVector &edges, int row, int col, int index);
    void markEdge(EdgesVector &edges, int row, int col, int index, bool used = true);
    int findHorizEdgeIndex(EdgesVector &edges, int row, int min_col, int max_col);
    int findVertEdgeIndex(EdgesVector &edges, int col, int min_row, int max_row);
    GraphEdge routeEdge(EdgesVector &horiz_edges, EdgesVector &vert_edges, Matrix<bool> &edge_valid,
                        GraphBlock &start, GraphBlock &end, QColor color);
    QPolygonF recalculatePolygon(QPolygonF polygon);
};

#endif // GRAPHVIEW_H
