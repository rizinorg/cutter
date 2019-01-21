#ifndef OVERVIEWVIEW_H
#define OVERVIEWVIEW_H

#include <QWidget>
#include <QPainter>
#include <QRect>
#include "widgets/GraphView.h"

class OverviewView : public GraphView
{
    Q_OBJECT

signals:
    void mouseMoved();
    void dataSet();

public:
    OverviewView(QWidget *parent);
    ~OverviewView() override;
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to) override;

    void paintEvent(QPaintEvent *event) override;
    QRectF rangeRect;

    void setData(int baseWidth, int baseHeight, std::unordered_map<ut64, GraphBlock> baseBlocks);

public slots:
    void refreshView();
    void colorsUpdatedSlot();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool mouseActive = false;
    QPointF initialDiff;
    void adjustScale();

    QColor disassemblyBackgroundColor;
    QColor graphNodeColor;
};

#endif // OVERVIEWVIEW_H
