#ifndef OVERVIEWVIEW_H
#define OVERVIEWVIEW_H

#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>
#include <QRect>

#include "widgets/GraphView.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/RichTextPainter.h"
#include "common/CutterSeekable.h"

class QTextEdit;

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

    bool isGraphEmpty();

    void paintEvent(QPaintEvent *event) override;
    QRectF rangeRect;
    QPoint graph_offset;

    void setData(int baseWidth, int baseHeight, std::unordered_map<ut64, GraphBlock> baseBlocks);

public slots:
    void refreshView();
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();

protected:

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool first_draw = true;

    bool mouseActive = false;

    // Font data
    CachedFontMetrics *mFontMetrics;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int baseline;
    bool emptyGraph;

    QPointF initialDiff;

    DisassemblyContextMenu *mMenu;

    void adjustScale();

    void initFont();
    void seekLocal(RVA addr, bool update_viewport = true);
    CutterSeekable *seekable = nullptr;
    QList<QShortcut *> shortcuts;
    QList<RVA> breakpoints;

    QColor disassemblyBackgroundColor;
    QColor disassemblySelectedBackgroundColor;
    QColor disassemblySelectionColor;
    QColor PCSelectionColor;
    QColor disassemblyTracedColor;
    QColor disassemblyTracedSelectionColor;
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

    QAction actionExportGraph;
    QAction actionSyncOffset;

    QLabel *emptyText = nullptr;
};

#endif // OVERVIEWVIEW_H
