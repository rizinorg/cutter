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

    enum class GraphExportType {
        Png, Jpeg, Svg, GVDot, GVJson,
        GVGif, GVPng, GVJpeg, GVPostScript, GVSvg,
        R2Gml, R2SDBKeyValue, R2Json
    };
    void exportGraph(QString filePath, GraphExportType type, QString graphCommand, RVA addres);
    void exportR2GraphvizGraph(QString filePath, QString type, QString graphCommand, RVA addres);
    void exportR2TextGraph(QString filePath, QString graphCommand, RVA address);
    static bool graphIsBitamp(GraphExportType type);
    /**
     * @brief Show export graph dialog.
     * @param defaultName - default file name in the export dialog
     * @param graphCommand - R2 graph commmand with type for example afC. Leave empty for non-r2 graphs. In such case
     * only direct image export will be available.
     * @param address - object address if relevant for \p graphCommand
     */
    void showExportGraphDialog(QString defaultName, QString graphCommand = "", RVA address = RVA_INVALID);

public slots:
    virtual void refreshView();
    void updateColors();
    void fontsUpdatedSlot();

    void zoom(QPointF mouseRelativePos, double velocity);
    void setZoom(QPointF mouseRelativePos, double scale);
    void zoomIn();
    void zoomOut();
    void zoomReset();

    virtual void showExportDialog();
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
    virtual void updateLayout();

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

    QAction actionExportGraph;

    GraphView::Layout graphLayout;
    QMenu *layoutMenu;
    QAction* horizontalLayoutAction;
private:
    void colorsUpdatedSlot();
};

#endif // CUTTER_GRAPHVIEW_H
