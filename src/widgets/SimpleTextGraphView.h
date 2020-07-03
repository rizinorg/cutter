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

    void setBlockSelectionEnabled(bool value);
public slots:
    void refreshView() override;
protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos) override;

    virtual void loadCurrentGraph() = 0;
    using CutterGraphView::addBlock;
    void addBlock(GraphLayout::GraphBlock block, const QString &content);


    struct BlockContent {
        QString text;
    };
    std::unordered_map<ut64, BlockContent> blockContent;

    QList<QShortcut *> shortcuts;
    QMenu *contextMenu;
    QAction copyAction;

    static const ut64 NO_BLOCK_SELECTED = RVA_INVALID;
    ut64 selectedBlock = RVA_INVALID;
    bool enableBlockSelection = true;
private:
    void copyBlockText();
};

#endif // SIMPLE_TEXT_GRAPHVIEW_H
