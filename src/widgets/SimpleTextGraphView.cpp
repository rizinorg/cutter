
#include "SimpleTextGraphView.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/Configuration.h"
#include "common/SyntaxHighlighter.h"
#include "common/Helpers.h"

#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QToolTip>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QClipboard>
#include <QApplication>
#include <QAction>

#include <cmath>

SimpleTextGraphView::SimpleTextGraphView(QWidget *parent, MainWindow *mainWindow)
    : CutterGraphView(parent),
      contextMenu(new QMenu(this)),
      addressableItemContextMenu(this, mainWindow),
      copyAction(tr("Copy"), this)
{
    copyAction.setShortcut(QKeySequence::StandardKey::Copy);
    copyAction.setShortcutContext(Qt::WidgetShortcut);
    connect(&copyAction, &QAction::triggered, this, &SimpleTextGraphView::copyBlockText);

    contextMenu->addAction(&copyAction);
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addMenu(layoutMenu);

    addressableItemContextMenu.insertAction(addressableItemContextMenu.actions().first(),
                                            &copyAction);
    addressableItemContextMenu.addSeparator();
    addressableItemContextMenu.addAction(&actionExportGraph);
    addressableItemContextMenu.addMenu(layoutMenu);

    addActions(addressableItemContextMenu.actions());
    addAction(&copyAction);
    enableAddresses(haveAddresses);
}

SimpleTextGraphView::~SimpleTextGraphView()
{
    for (QShortcut *shortcut : shortcuts) {
        delete shortcut;
    }
}

void SimpleTextGraphView::refreshView()
{
    initFont();
    setLayoutConfig(getLayoutConfig());
    saveCurrentBlock();
    loadCurrentGraph();
    if (blocks.find(selectedBlock) == blocks.end()) {
        selectedBlock = NO_BLOCK_SELECTED;
    }
    restoreCurrentBlock();
    emit viewRefreshed();
}

void SimpleTextGraphView::selectBlockWithId(ut64 blockId)
{
    if (!enableBlockSelection) {
        return;
    }
    auto contentIt = blockContent.find(blockId);
    if (contentIt != blockContent.end()) {
        selectedBlock = blockId;
        if (haveAddresses) {
            addressableItemContextMenu.setTarget(contentIt->second.address, contentIt->second.text);
        }
        viewport()->update();
    } else {
        selectedBlock = NO_BLOCK_SELECTED;
    }
}

void SimpleTextGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    // Render node
    auto &content = blockContent[block.entry];

    p.setPen(QColor(0, 0, 0, 0));
    p.setBrush(QColor(0, 0, 0, 100));
    p.setPen(QPen(graphNodeColor, 1));

    bool blockSelected = interactive && (block.entry == selectedBlock);
    if (blockSelected) {
        p.setBrush(disassemblySelectedBackgroundColor);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }
    // Draw basic block background
    p.drawRect(blockRect);

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    QRect screenChar = transform.mapRect(QRect(0, 0, ACharWidth, charHeight));

    if (screenChar.width() < Config()->getGraphMinFontSize()) {
        return;
    }

    p.setPen(palette().color(QPalette::WindowText));
    // Render node text
    auto x = block.x + padding;
    int y = block.y + padding + p.fontMetrics().ascent();
    p.drawText(QPoint(x, y), content.text);
}

GraphView::EdgeConfiguration SimpleTextGraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                                    GraphView::GraphBlock *to,
                                                                    bool interactive)
{
    EdgeConfiguration ec;
    ec.color = jmpColor;
    ec.start_arrow = false;
    ec.end_arrow = true;
    if (interactive && (selectedBlock == from.entry || selectedBlock == to->entry)) {
        ec.width_scale = 2.0;
    }
    return ec;
}

void SimpleTextGraphView::setBlockSelectionEnabled(bool value)
{
    enableBlockSelection = value;
    if (!value) {
        selectedBlock = NO_BLOCK_SELECTED;
    }
}

void SimpleTextGraphView::addBlock(GraphLayout::GraphBlock block, const QString &text, RVA address)
{
    auto &content = blockContent[block.entry];
    content.text = text;
    content.address = address;

    int height = 1;
    int width = mFontMetrics->width(text);
    block.width = static_cast<int>(width + padding);
    block.height = (height * charHeight) + padding;
    GraphView::addBlock(std::move(block));
}

void SimpleTextGraphView::enableAddresses(bool enabled)
{
    haveAddresses = enabled;
    if (!enabled) {
        addressableItemContextMenu.clearTarget();
        // Clearing addreassable item context menu disables all the actions inside it including
        // extra ones added by SimpleTextGraphView. Re-enable them because they are in the regular
        // context menu as well and shouldn't be disabled.
        for (auto action : contextMenu->actions()) {
            action->setEnabled(true);
        }
    };
}

void SimpleTextGraphView::copyBlockText()
{
    auto blockIt = blockContent.find(selectedBlock);
    if (blockIt != blockContent.end()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(blockIt->second.text);
    }
}

void SimpleTextGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted() && event->reason() != QContextMenuEvent::Mouse && enableBlockSelection
        && selectedBlock != NO_BLOCK_SELECTED) {
        auto blockIt = blocks.find(selectedBlock);
        if (blockIt != blocks.end()) {
            blockContextMenuRequested(blockIt->second, event, {});
        }
    }
    if (!event->isAccepted()) {
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void SimpleTextGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                                    QContextMenuEvent *event, QPoint /*pos*/)
{
    if (haveAddresses) {
        const auto &content = blockContent[block.entry];
        addressableItemContextMenu.setTarget(content.address, content.text);
        QPoint pos = event->globalPos();

        if (event->reason() != QContextMenuEvent::Mouse) {
            QPoint blockPosition(block.x + block.width / 2, block.y + block.height / 2);
            blockPosition = logicalToViewCoordinates(blockPosition);
            if (viewport()->rect().contains(blockPosition)) {
                pos = mapToGlobal(blockPosition);
            }
        }
        addressableItemContextMenu.exec(pos);
        event->accept();
    }
}

void SimpleTextGraphView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event,
                                         QPoint /*pos*/)
{
    if (haveAddresses) {
        QToolTip::showText(event->globalPos(), RAddressString(blockContent[block.entry].address));
    }
}

void SimpleTextGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                       QPoint /*pos*/)
{
    if ((event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        && enableBlockSelection) {
        selectBlockWithId(block.entry);
    }
}

void SimpleTextGraphView::restoreCurrentBlock()
{
    if (enableBlockSelection) {
        auto blockIt = blocks.find(selectedBlock);
        if (blockIt != blocks.end()) {
            showBlock(blockIt->second, true);
        }
    }
}

void SimpleTextGraphView::paintEvent(QPaintEvent *event)
{
    // SimpleTextGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}
