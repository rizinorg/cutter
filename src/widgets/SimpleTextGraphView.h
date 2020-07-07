#ifndef SIMPLE_TEXT_GRAPHVIEW_H
#define SIMPLE_TEXT_GRAPHVIEW_H

// Based on the DisassemblerGraphView from x64dbg

#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

#include "widgets/CutterGraphView.h"
#include "menus/AddressableItemContextMenu.h"
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
    void selectBlockWithId(ut64 blockId);
protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void blockContextMenuRequested(GraphView::GraphBlock &block, QContextMenuEvent *event, QPoint pos) override;
    void blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos)override;
    void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos) override;

    void restoreCurrentBlock() override;

    virtual void loadCurrentGraph() = 0;
    using CutterGraphView::addBlock;
    void addBlock(GraphLayout::GraphBlock block, const QString &content, RVA address = RVA_INVALID);
    void enableAddresses(bool enabled);

    struct BlockContent {
        QString text;
        RVA address;
    };
    std::unordered_map<ut64, BlockContent> blockContent;

    QList<QShortcut *> shortcuts;
    QMenu *contextMenu;
    AddressableItemContextMenu addressableItemContextMenu;
    QAction copyAction;

    static const ut64 NO_BLOCK_SELECTED = RVA_INVALID;
    ut64 selectedBlock = RVA_INVALID;
    bool enableBlockSelection = true;
    bool haveAddresses = false;
private:
    void copyBlockText();
};

#endif // SIMPLE_TEXT_GRAPHVIEW_H
