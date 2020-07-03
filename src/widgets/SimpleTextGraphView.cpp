
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
      graphLayout(GraphView::Layout::GridMedium),
      contextMenu(new QMenu(this))
{
    // Signals that require a refresh all
    connect(Core(), &CutterCore::refreshAll, this, &SimpleTextGraphView::refreshView);
    connect(Core(), &CutterCore::graphOptionsChanged, this, &SimpleTextGraphView::refreshView);

    // Context menu that applies to everything
    static const std::pair<QString, GraphView::Layout> LAYOUT_CONFIG[] = {
        {tr("Grid narrow"), GraphView::Layout::GridNarrow}
        , {tr("Grid medium"), GraphView::Layout::GridMedium}
        , {tr("Grid wide"), GraphView::Layout::GridWide}
#ifdef CUTTER_ENABLE_GRAPHVIZ
        , {tr("Graphviz polyline"), GraphView::Layout::GraphvizPolyline}
        , {tr("Graphviz ortho"), GraphView::Layout::GraphvizOrtho}
#endif
    };
    auto layoutMenu = contextMenu->addMenu(tr("Layout"));
    horizontalLayoutAction = layoutMenu->addAction(tr("Horizontal"));
    horizontalLayoutAction->setCheckable(true);
    layoutMenu->addSeparator();
    connect(horizontalLayoutAction, &QAction::toggled, this, &SimpleTextGraphView::updateLayout);
    QActionGroup *layoutGroup = new QActionGroup(layoutMenu);
    for (auto &item : LAYOUT_CONFIG) {
        auto action = layoutGroup->addAction(item.first);
        action->setCheckable(true);
        GraphView::Layout layout = item.second;
        connect(action, &QAction::triggered, this, [this, layout]() {
            this->graphLayout = layout;
            updateLayout();
        });
        if (layout == this->graphLayout) {
            action->setChecked(true);
        }
    }
    layoutMenu->addActions(layoutGroup->actions());
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

void SimpleTextGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool /*interactive*/)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    const qreal padding = 2 * charWidth;

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    // Render node
    auto &content = blockContent[block.entry];

    p.setPen(QColor(0, 0, 0, 0));
    p.setBrush(QColor(0, 0, 0, 100));
    p.setPen(QPen(graphNodeColor, 1));
    p.setBrush(disassemblyBackgroundColor);

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
    int y = block.y + getTextOffset(0).y();
    p.drawText(QPoint(x, y), content.text);
    y += charHeight;
}

GraphView::EdgeConfiguration SimpleTextGraphView::edgeConfiguration(GraphView::GraphBlock & /*from*/,
                                                                      GraphView::GraphBlock * /*to*/,
                                                                      bool /*interactive*/)
{
    EdgeConfiguration ec;
    ec.color = jmpColor;
    ec.start_arrow = false;
    ec.end_arrow = true;
    return ec;
}

void SimpleTextGraphView::addBlock(GraphLayout::GraphBlock block, const QString &content)
{
    blockContent[block.entry].text = content;
    int height = 1;
    int width = mFontMetrics->width(content);
    int extra = static_cast<int>(4 * charWidth + 4);
    block.width = static_cast<int>(width + extra + charWidth);
    block.height = (height * charHeight) + extra;
    GraphView::addBlock(std::move(block));
}

void SimpleTextGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        QMenu menu(this);
        menu.addAction(&actionExportGraph);
        menu.addMenu(layoutMenu);
        menu.exec(event->globalPos());
    }
}

void SimpleTextGraphView::paintEvent(QPaintEvent *event)
{
    // SimpleTextGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}
