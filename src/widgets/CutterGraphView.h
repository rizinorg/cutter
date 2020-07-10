#ifndef CUTTER_GRAPHVIEW_H
#define CUTTER_GRAPHVIEW_H


#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

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
        Png, Jpeg, Svg, GVDot, GVJson,
        GVGif, GVPng, GVJpeg, GVPostScript, GVSvg,
        R2Gml, R2SDBKeyValue, R2Json
    };
    /**
     * @brief Export graph to a file in the specified format
     * @param filePath
     * @param type export type, GV* and R2* types require \p graphCommand
     * @param graphCommand r2 graph printing command without type, not required for direct image export
     * @param address object address for commands like agf
     */
    void exportGraph(QString filePath, GraphExportType type, QString graphCommand = "", RVA address = RVA_INVALID);
    /**
     * @brief Export image using r2 ag*w command and graphviz.
     * Requires graphviz dot executable in the path.
     *
     * @param filePath output file path
     * @param type image format as expected by "e graph.gv.format"
     * @param graphCommand r2 command without type, for example agf
     * @param address object address if required by command
     */
    void exportR2GraphvizGraph(QString filePath, QString type, QString graphCommand, RVA address);
    /**
     * @brief Export graph in one of the text formats supported by r2 json, gml, SDB key-value
     * @param filePath output file path
     * @param graphCommand graph command including the format, example "agfd" or "agfg"
     * @param address object address if required by command
     */
    void exportR2TextGraph(QString filePath, QString graphCommand, RVA address);
    static bool graphIsBitamp(GraphExportType type);
    /**
     * @brief Show graph export dialog.
     * @param defaultName - default file name in the export dialog
     * @param graphCommand - R2 graph commmand with graph type and without export type, for example afC. Leave empty
     * for non-r2 graphs. In such case only direct image export will be available.
     * @param address - object address if relevant for \p graphCommand
     */
    void showExportGraphDialog(QString defaultName, QString graphCommand = "",
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
     * @brief Show the export file dialog. Override this to support r2 based export formats.
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

    /**
     * @brief Save the the currently viewed or displayed block.
     * Called before reloading graph. Override to this to implement graph specific logic for what block is selected.
     * Default implementation does nothing.
     */
    virtual void saveCurrentBlock();
    /**
     * @brief Restore view focus and block last saved using saveCurrentBlock().
     * Called after the graph is reloaded. Default implementation does nothing. Can center the view if the new graph
     * displays completely different content and the matching node doesn't exist.
     */
    virtual void restoreCurrentBlock();

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
    QAction *horizontalLayoutAction;
private:
    void colorsUpdatedSlot();
};

#endif // CUTTER_GRAPHVIEW_H
