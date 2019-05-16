#ifndef OVERVIEWVIEW_H
#define OVERVIEWVIEW_H

#include <QWidget>
#include <QPainter>
#include <QRect>
#include "widgets/GraphView.h"
#include "widgets/DisassemblerGraphView.h"

class OverviewView : public GraphView
{
    Q_OBJECT

signals:
    /**
     * @brief signal when mouse is pressed or moved so that
     * Graph can refresh its contents corresponded with Overview
     */
    void mouseMoved();

public:
    OverviewView(QWidget *parent);
    ~OverviewView() override;

    /**
     * @brief Graph access this function to set minimum set of the data
     * @param baseWidth width of Graph when it computed the blocks
     * @param baseHeigh height of Graph when it computed the blocks
     * @param baseBlocks computed blocks passed by Graph
     * @param baseEdgeConfigurations computed by DisassamblerGraphview
     */
    void setData(int baseWidth, int baseHeight, std::unordered_map<ut64, GraphBlock> baseBlocks,
                 DisassemblerGraphView::EdgeConfigurationMapping baseEdgeConfigurations);

public slots:
    /**
     * @brief scale and center all nodes in, then run update
     */
    void refreshView();

private slots:
    /**
     * @brief update Colors.
     * for example this will be called when the theme is changed.
     */
    void colorsUpdatedSlot();

protected:
    /**
     * @brief mousePressEvent to start moving the rect.
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * @brief mouseReleaseEvent to tell not to move the rect anymore.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;
    /**
     * @brief mouseMoveEvent to move the rect.
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * @brief override this to prevent scrolling
     */
    void wheelEvent(QWheelEvent *event) override;

    /**
     * @brief override the paintEvent to draw the rect on Overview
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief this will be handled in mouse events to move the rect properly
     * along with the mouse.
     */
    bool mouseActive = false;

    /**
     * @brief save the initial distance
     * between the point where the mouse was pressed and the point of the rect
     * so as to change the rect point properly along with mouse.
     */
    QPointF initialDiff;

    /**
     * @brief a rect on Overview to show where you are on Graph
     */
    QRectF rangeRect;

    /**
     * @brief calculate the scale to fit the all nodes in and center them in the viewport
     */
    void scaleAndCenter();

    /**
     * @brief draw the computed blocks passed by Graph
     */
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block) override;

    /**
     * @brief override the edgeConfiguration so as to
     * adjust the width of the edges by the scale
     * @return EdgeConfiguration
     */
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to) override;

    /**
     * @brief base background color changing depending on the theme
     */
    QColor disassemblyBackgroundColor;

    /**
     * @brief color for each node changing depending on the theme
     */
    QColor graphNodeColor;

    /**
     * @brief edgeConfigurations edge styles computed by DisassemblerGraphView
     */
    DisassemblerGraphView::EdgeConfigurationMapping edgeConfigurations;

public:
    QRectF getRangeRect()       { return rangeRect; }
    void setRangeRect(QRectF rect);
};

#endif // OVERVIEWVIEW_H
