#ifndef SIMPLE_TEXT_GRAPHVIEW_H
#define SIMPLE_TEXT_GRAPHVIEW_H

// Based on the DisassemblerGraphView from x64dbg

#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

#include "widgets/CutterGraphView.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/RichTextPainter.h"
#include "common/CutterSeekable.h"

class SimpleTextGraphView : public CutterGraphView
{
    Q_OBJECT
public:
    SimpleTextGraphView(QWidget *parent, MainWindow *mainWindow);
    ~SimpleTextGraphView() override;
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to,
                                                           bool interactive) override;

    enum class GraphExportType {
        Png, Jpeg, Svg, GVDot, GVJson,
        GVGif, GVPng, GVJpeg, GVPostScript, GVSvg
    };
    //void exportGraph(QString filePath, GraphExportType type);
    //void exportR2GraphvizGraph(QString filePath, QString type);
public slots:
    void refreshView();
protected:
    void paintEvent(QPaintEvent *event) override;
    void updateLayout();
    virtual void loadCurrentGraph() = 0;

    struct BlockContent {
        QString text;
    };
    std::unordered_map<ut64, BlockContent> blockContent;

    GraphView::Layout graphLayout;

    QMenu *contextMenu;
    QAction* horizontalLayoutAction;

    void prepareGraphNode(GraphBlock &block);

    QList<QShortcut *> shortcuts;

    QAction actionExportGraph;
};

#endif // SIMPLE_TEXT_GRAPHVIEW_H
