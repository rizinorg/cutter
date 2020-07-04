
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
      copyAction(tr("Copy"), this)
{
    // Signals that require a refresh all
    connect(Core(), &CutterCore::refreshAll, this, &SimpleTextGraphView::refreshView);
    connect(Core(), &CutterCore::graphOptionsChanged, this, &SimpleTextGraphView::refreshView);

    copyAction.setShortcut(QKeySequence::StandardKey::Copy);
    copyAction.setShortcutContext(Qt::WidgetShortcut);
    connect(&copyAction, &QAction::triggered, this, &SimpleTextGraphView::copyBlockText);

    contextMenu->addAction(&copyAction);
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addMenu(layoutMenu);

    addAction(&copyAction);
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
    loadCurrentGraph();
    emit viewRefreshed();
}

void SimpleTextGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    const qreal padding =  charWidth;

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
    QRect screenChar = transform.mapRect(QRect(0, 0, charWidth, charHeight));

    if (screenChar.width() * qhelpers::devicePixelRatio(p.device()) < 4) {
        return;
    }

    p.setPen(mLabelColor);
    // Render node text
    auto x = block.x + padding;
    int y = block.y +  padding + p.fontMetrics().ascent();
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

void SimpleTextGraphView::addBlock(GraphLayout::GraphBlock block, const QString &content)
{
    blockContent[block.entry].text = content;
    int height = 1;
    int width = mFontMetrics->width(content);
    int extra = static_cast<int>(2 * charWidth);
    block.width = static_cast<int>(width + extra);
    block.height = (height * charHeight) + extra;
    GraphView::addBlock(std::move(block));
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
    if (!event->isAccepted()) {
        contextMenu->exec(event->globalPos());
    }
}

void SimpleTextGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint /*pos*/)
{
    if ((event->button() == Qt::LeftButton || event->button() == Qt::RightButton) && enableBlockSelection) {
        selectedBlock = block.entry;
        viewport()->update();
    }
}

void SimpleTextGraphView::paintEvent(QPaintEvent *event)
{
    // SimpleTextGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}
