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

/**
 * @brief Graphview with nodes containing simple plaintext labels.
 */
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

    /**
     * @brief Enable or disable block selection.
     * Selecting a block highlights it and allows copying the label. Enabled by default.
     * @param value
     */
    void setBlockSelectionEnabled(bool value);
public slots:
    void refreshView() override;
    /**
     * @brief Select a given block. Requires block selection to be enabled.
     */
    void selectBlockWithId(ut64 blockId);
protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void blockContextMenuRequested(GraphView::GraphBlock &block, QContextMenuEvent *event,
                                   QPoint pos) override;
    void blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos)override;
    void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos) override;

    void restoreCurrentBlock() override;

    /**
     * @brief Load the graph to be displayed.
     * Needs to cleanup the old graph and use addBlock() to create new nodes.
     */
    virtual void loadCurrentGraph() = 0;
    void addBlock(GraphLayout::GraphBlock block, const QString &content, RVA address = RVA_INVALID);
    /**
     * @brief Enable or disable address interactions for nodes.
     * If enabled node addresses need to be specified when calling addBlock(). Adds address related
     * items to the node context menu. By default disabled.
     * @param enabled
     */
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
    ut64 selectedBlock = NO_BLOCK_SELECTED;
    bool enableBlockSelection = true;
    bool haveAddresses = false;
private:
    void copyBlockText();
};

#endif // SIMPLE_TEXT_GRAPHVIEW_H
