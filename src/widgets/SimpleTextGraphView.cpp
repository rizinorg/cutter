
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

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));

    // Export Graph menu
    actionExportGraph.setText(tr("Export Graph"));
    connect(&actionExportGraph, SIGNAL(triggered(bool)), this, SLOT(on_actionExportGraph_triggered()));

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


    initFont();
    colorsUpdatedSlot();

    this->scale_thickness_multiplier = true;
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

void SimpleTextGraphView::loadCurrentGraph()
{
    /*
    TempConfig tempConfig;
    tempConfig.set("scr.color", COLOR_MODE_16M)
    .set("asm.bb.line", false)
    .set("asm.lines", false)
    .set("asm.lines.fcn", false);

    QJsonArray functions;
    RAnalFunction *fcn = Core()->functionIn(seekable->getOffset());
    if (fcn) {
        currentFcnAddr = fcn->addr;
        QJsonDocument functionsDoc = Core()->cmdj("agJ " + RAddressString(fcn->addr));
        functions = functionsDoc.array();
    }

    disassembly_blocks.clear();
    blocks.clear();

    if (highlight_token) {
        delete highlight_token;
        highlight_token = nullptr;
    }

    emptyGraph = functions.isEmpty();
    if (emptyGraph) {
        // If there's no function to print, just add a message
        if (!emptyText) {
            emptyText = new QLabel(this);
            emptyText->setText(tr("No function detected. Cannot display graph."));
            emptyText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            layout()->addWidget(emptyText);
            layout()->setAlignment(emptyText, Qt::AlignHCenter);
        }
        emptyText->setVisible(true);
    } else if (emptyText) {
        emptyText->setVisible(false);
    }
    // Refresh global "empty graph" variable so other widget know there is nothing to show here
    Core()->setGraphEmpty(emptyGraph);

    QJsonValue funcRef = functions.first();
    QJsonObject func = funcRef.toObject();

    windowTitle = tr("Graph");
    QString funcName = func["name"].toString().trimmed();
    if (emptyGraph) {
        windowTitle += " (Empty)";
    } else if (!funcName.isEmpty()) {
        windowTitle += " (" + funcName + ")";
    }
    emit nameChanged(windowTitle);

    RVA entry = func["offset"].toVariant().toULongLong();

    setEntry(entry);
    for (const QJsonValueRef &value : func["blocks"].toArray()) {
        QJsonObject block = value.toObject();
        RVA block_entry = block["offset"].toVariant().toULongLong();
        RVA block_size = block["size"].toVariant().toULongLong();
        RVA block_fail = block["fail"].toVariant().toULongLong();
        RVA block_jump = block["jump"].toVariant().toULongLong();

        DisassemblyBlock db;
        GraphBlock gb;
        gb.entry = block_entry;
        db.entry = block_entry;
        db.true_path = RVA_INVALID;
        db.false_path = RVA_INVALID;
        if (block_fail) {
            db.false_path = block_fail;
            gb.edges.emplace_back(block_fail);
        }
        if (block_jump) {
            if (block_fail) {
                db.true_path = block_jump;
            }
            gb.edges.emplace_back(block_jump);
        }

        QJsonObject switchOp = block["switchop"].toObject();
        if (!switchOp.isEmpty()) {
            QJsonArray caseArray = switchOp["cases"].toArray();
            for (QJsonValue caseOpValue : caseArray) {
                QJsonObject caseOp = caseOpValue.toObject();
                bool ok;
                RVA caseJump = caseOp["jump"].toVariant().toULongLong(&ok);
                if (!ok) {
                    continue;
                }
                gb.edges.emplace_back(caseJump);
            }
        }

        QJsonArray opArray = block["ops"].toArray();
        for (int opIndex = 0; opIndex < opArray.size(); opIndex++) {
            QJsonObject op = opArray[opIndex].toObject();
            Instr i;
            i.addr = op["offset"].toVariant().toULongLong();

            if (opIndex < opArray.size() - 1) {
                // get instruction size from distance to next instruction ...
                RVA nextOffset = opArray[opIndex + 1].toObject()["offset"].toVariant().toULongLong();
                i.size = nextOffset - i.addr;
            } else {
                // or to the end of the block.
                i.size = (block_entry + block_size) - i.addr;
            }

            QTextDocument textDoc;
            textDoc.setHtml(CutterCore::ansiEscapeToHtml(op["text"].toString()));

            i.plainText = textDoc.toPlainText();

            RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);
            //Colors::colorizeAssembly(richText, textDoc.toPlainText(), 0);

            bool cropped;
            int blockLength = Config()->getGraphBlockMaxChars() + Core()->getConfigb("asm.bytes") * 24 +
                              Core()->getConfigb("asm.emu") * 10;
            i.text = Text(RichTextPainter::cropped(richText, blockLength, "...", &cropped));
            if (cropped)
                i.fullText = richText;
            else
                i.fullText = Text();
            db.instrs.push_back(i);
        }
        disassembly_blocks[db.entry] = db;
        prepareGraphNode(gb);

        addBlock(gb);
    }
    cleanupEdges();

    if (!func["blocks"].toArray().isEmpty()) {
        computeGraphPlacement();
    }*/
}


void SimpleTextGraphView::prepareGraphNode(GraphBlock &block)
{
    auto &db = blockContent[block.entry];
    int height = 1;
    int width = mFontMetrics->width(db.text);
    int extra = static_cast<int>(4 * charWidth + 4);
    block.width = static_cast<int>(width + extra + charWidth);
    block.height = (height * charHeight) + extra;
}

void SimpleTextGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
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

    const int firstInstructionY = block.y + getTextOffset(0).y();

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    QRect screenChar = transform.mapRect(QRect(0, 0, charWidth, charHeight));

    if (screenChar.width() * qhelpers::devicePixelRatio(p.device()) < 4) {
        return;
    }

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
