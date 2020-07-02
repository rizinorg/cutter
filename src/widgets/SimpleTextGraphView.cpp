
#include "SimpleTextGraphView.h"
#include "common/CutterSeekable.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/CachedFontMetrics.h"
#include "common/TempConfig.h"
#include "common/SyntaxHighlighter.h"
#include "common/BasicBlockHighlighter.h"
#include "common/BasicInstructionHighlighter.h"
#include "dialogs/MultitypeFileSaveDialog.h"
#include "common/Helpers.h"

#include <QColorDialog>
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QToolTip>
#include <QTextDocument>
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QClipboard>
#include <QApplication>
#include <QAction>

#include <cmath>

SimpleTextGraphView::SimpleTextGraphView(QWidget *parent, MainWindow *mainWindow)
    : CutterGraphView(parent),
      graphLayout(GraphView::Layout::GridMedium),
      contextMenu(new QMenu(this)),
      actionExportGraph(this)
{
    // Signals that require a refresh all
    connect(Core(), &CutterCore::refreshAll, this, &SimpleTextGraphView::refreshView);
    connect(Core(), &CutterCore::graphOptionsChanged, this, &SimpleTextGraphView::refreshView);

    // Export Graph menu
    actionExportGraph.setText(tr("Export Graph"));
    //connect(&actionExportGraph, SIGNAL(triggered(bool)), this, SLOT(on_actionExportGraph_triggered()));

    // Context menu that applies to everything
    contextMenu->addAction(&actionExportGraph);
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

/*
Q_DECLARE_METATYPE(SimpleTextGraphView::GraphExportType);

void SimpleTextGraphView::on_actionExportGraph_triggered()
{
    QVector<MultitypeFileSaveDialog::TypeDescription> types = {
        {tr("PNG (*.png)"), "png", QVariant::fromValue(GraphExportType::Png)},
        {tr("JPEG (*.jpg)"), "jpg", QVariant::fromValue(GraphExportType::Jpeg)},
        {tr("SVG (*.svg)"), "svg", QVariant::fromValue(GraphExportType::Svg)}
    };
    bool hasGraphviz = !QStandardPaths::findExecutable("dot").isEmpty()
                       || !QStandardPaths::findExecutable("xdot").isEmpty();
    if (hasGraphviz) {
        types.append({
            {tr("Graphviz dot (*.dot)"), "dot", QVariant::fromValue(GraphExportType::GVDot)},
            {tr("Graphviz json (*.json)"), "json", QVariant::fromValue(GraphExportType::GVJson)},
            {tr("Graphviz gif (*.gif)"), "gif", QVariant::fromValue(GraphExportType::GVGif)},
            {tr("Graphviz png (*.png)"), "png", QVariant::fromValue(GraphExportType::GVPng)},
            {tr("Graphviz jpg (*.jpg)"), "jpg", QVariant::fromValue(GraphExportType::GVJpeg)},
            {tr("Graphviz PostScript (*.ps)"), "ps", QVariant::fromValue(GraphExportType::GVPostScript)},
            {tr("Graphviz svg (*.svg)"), "svg", QVariant::fromValue(GraphExportType::GVSvg)}
        });
    }

    QString defaultName = "graph";
    if (auto f = Core()->functionIn(currentFcnAddr)) {
        QString functionName = f->name;
        // don't confuse image type guessing and make c++ names somewhat usable
        functionName.replace(QRegularExpression("[.:]"), "_");
        functionName.remove(QRegularExpression("[^a-zA-Z0-9_].*"));
        if (!functionName.isEmpty()) {
            defaultName = functionName;
        }
    }


    MultitypeFileSaveDialog dialog(this, tr("Export Graph"));
    dialog.setTypes(types);
    dialog.selectFile(defaultName);
    if (!dialog.exec())
        return;

    auto selectedType = dialog.selectedType();
    if (!selectedType.data.canConvert<GraphExportType>()) {
        qWarning() << "Bad selected type, should not happen.";
        return;
    }
    QString filePath = dialog.selectedFiles().first();
    exportGraph(filePath, selectedType.data.value<GraphExportType>());

}*/


void SimpleTextGraphView::updateLayout()
{
    setGraphLayout(GraphView::makeGraphLayout(graphLayout, horizontalLayoutAction->isChecked()));
    setLayoutConfig(getLayoutConfig());
    computeGraphPlacement();
    emit viewRefreshed();
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
/*
void SimpleTextGraphView::exportGraph(QString filePath, GraphExportType type)
{
    bool graphTransparent = Config()->getBitmapTransparentState();
    double graphScaleFactor = Config()->getBitmapExportScaleFactor();
    switch (type) {
    case GraphExportType::Png:
        this->saveAsBitmap(filePath, "png", graphScaleFactor, graphTransparent);
        break;
    case GraphExportType::Jpeg:
        this->saveAsBitmap(filePath, "jpg", graphScaleFactor, false);
        break;
    case GraphExportType::Svg:
        this->saveAsSvg(filePath);
        break;

    case GraphExportType::GVDot: {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Can't open file";
            return;
        }
        QTextStream fileOut(&file);
        fileOut << Core()->cmdRaw(QString("agfd 0x%1").arg(currentFcnAddr, 0, 16));
    }
    break;

    case GraphExportType::GVJson:
        exportR2GraphvizGraph(filePath, "json");
        break;
    case GraphExportType::GVGif:
        exportR2GraphvizGraph(filePath, "gif");
        break;
    case GraphExportType::GVPng:
        exportR2GraphvizGraph(filePath, "png");
        break;
    case GraphExportType::GVJpeg:
        exportR2GraphvizGraph(filePath, "jpg");
        break;
    case GraphExportType::GVPostScript:
        exportR2GraphvizGraph(filePath, "ps");
        break;
    case GraphExportType::GVSvg:
        exportR2GraphvizGraph(filePath, "svg");
        break;
    }
}*/

/*void SimpleTextGraphView::exportR2GraphvizGraph(QString filePath, QString type)
{
    TempConfig tempConfig;
    tempConfig.set("graph.gv.format", type);
    qWarning() << Core()->cmdRawAt(QString("agfw \"%1\"")
                                 .arg(filePath),
                                 currentFcnAddr);
}*/


void SimpleTextGraphView::paintEvent(QPaintEvent *event)
{
    // SimpleTextGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}
