#ifndef CUTTER_GRAPHVIEW_H
#define CUTTER_GRAPHVIEW_H


#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

#include "widgets/GraphView.h"
#include "common/CachedFontMetrics.h"

class CutterGraphView : public GraphView
{
    Q_OBJECT
public:
    CutterGraphView(QWidget *parent);
    virtual bool event(QEvent *event) override;

public slots:
    virtual void refreshView();
    void updateColors();
    void fontsUpdatedSlot();

    void zoom(QPointF mouseRelativePos, double velocity);
    void setZoom(QPointF mouseRelativePos, double scale);
    void zoomIn();
    void zoomOut();
    void zoomReset();
signals:
    void viewRefreshed();
    void viewZoomed();
    void graphMoved();
    void resized();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void initFont();
    QPoint getTextOffset(int line) const;
    GraphLayout::LayoutConfig getLayoutConfig();

    // Font data
    std::unique_ptr<CachedFontMetrics<qreal>> mFontMetrics;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int baseline;

    // colors
    QColor disassemblyBackgroundColor;
    QColor disassemblySelectedBackgroundColor;
    QColor disassemblySelectionColor;
    QColor PCSelectionColor;
    QColor jmpColor;
    QColor brtrueColor;
    QColor brfalseColor;
    QColor retShadowColor;
    QColor indirectcallShadowColor;
    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor graphNodeColor;
    QColor mAddressColor;
    QColor mAddressBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointColor;
    QColor mDisabledBreakpointColor;
private:
    void colorsUpdatedSlot();
};

#endif // GRAPHVIEW_H
