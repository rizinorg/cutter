#ifndef CUTTER_GRAPHVIEW_H
#define CUTTER_GRAPHVIEW_H

#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

#include <QGestureEvent>

#include "widgets/GraphView.h"
#include "common/CachedFontMetrics.h"

/**
 * @brief Common Cutter specific graph functionality.
 */
class CutterGraphView : public GraphView
{
    Q_OBJECT
public:
    CutterGraphView(QWidget *parent);
    virtual bool event(QEvent *event) override;

    enum class GraphExportType {
        Png,
        Jpeg,
        Svg,
        GVDot,
        GVJson,
        GVGif,
        GVPng,
        GVJpeg,
        GVPostScript,
        GVSvg,
        GVPdf,
        RzGml,
        RzJson
    };
    /**
     * @brief Export graph to a file in the specified format
     * @param filePath - output file path
     * @param exportType - export type, GV* and Rz* types require \p graphCommand
     * @param graphType - graph type, example RZ_CORE_GRAPH_TYPE_FUNCALL or
     * RZ_CORE_GRAPH_TYPE_IMPORT
     * @param address - object address (if global set it to RVA_INVALID)
     */
    void exportGraph(QString filePath, GraphExportType exportType, RzCoreGraphType graphType,
                     RVA address = RVA_INVALID);

    /**
     * @brief Export graph in one of the text formats supported by rizin json, gml, SDB key-value
     * @param filePath - output file path
     * @param type - graph type, example RZ_CORE_GRAPH_TYPE_FUNCALL or RZ_CORE_GRAPH_TYPE_IMPORT
     * @param format - graph format, example RZ_CORE_GRAPH_FORMAT_DOT or RZ_CORE_GRAPH_FORMAT_GML
     * @param address - object address (if global set it to RVA_INVALID)
     */
    void exportRzTextGraph(QString filePath, RzCoreGraphType type, RzCoreGraphFormat format,
                           RVA address);
    static bool graphIsBitamp(GraphExportType type);
    /**
     * @brief Show graph export dialog.
     * @param defaultName - default file name in the export dialog
     * @param type - graph type, example RZ_CORE_GRAPH_TYPE_FUNCALL or RZ_CORE_GRAPH_TYPE_IMPORT
     * @param address - object address (if global set it to RVA_INVALID)
     */
    void showExportGraphDialog(QString defaultName, RzCoreGraphType type,
                               RVA address = RVA_INVALID);

public slots:
    virtual void refreshView();
    void updateColors();
    void fontsUpdatedSlot();

    void zoom(QPointF mouseRelativePos, double velocity);
    void setZoom(QPointF mouseRelativePos, double scale);
    void zoomIn();
    void zoomOut();
    void zoomReset();

    /**
     * @brief Show the export file dialog. Override this to support rizin based export formats.
     */
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
    bool gestureEvent(QGestureEvent *event) override;

    /**
     * @brief Save the the currently viewed or displayed block.
     * Called before reloading graph. Override to this to implement graph specific logic for what
     * block is selected. Default implementation does nothing.
     */
    virtual void saveCurrentBlock();
    /**
     * @brief Restore view focus and block last saved using saveCurrentBlock().
     * Called after the graph is reloaded. Default implementation does nothing. Can center the view
     * if the new graph displays completely different content and the matching node doesn't exist.
     */
    virtual void restoreCurrentBlock();

    void initFont();
    QPoint getTextOffset(int line) const;
    GraphLayout::LayoutConfig getLayoutConfig();
    virtual void updateLayout();

    // Font data
    std::unique_ptr<CachedFontMetrics<qreal>> mFontMetrics;
    qreal ACharWidth; // width of character A
    int charHeight;
    int charOffset;
    int baseline;
    qreal padding;

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
    QAction *horizontalLayoutAction;

private:
    void colorsUpdatedSlot();
};

#endif // CUTTER_GRAPHVIEW_H
