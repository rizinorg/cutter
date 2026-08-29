#include "DiffGraphView.h"

#include <QVBoxLayout>

#include <DisassemblyPreview.h>
#include <dialogs/MultitypeFileSaveDialog.h>

DiffGraphView::DiffGraphView(CutterDiff *cutterDiff, QWidget *parent)
    : CutterGraphView(parent), contextMenu(new QMenu(this)), cutterDiff(cutterDiff)
{
    // colors update
    // relevant refresh signals
    // Navigation shortcuts
    actionCenter = new QAction("Center", this);
    installEventFilter(this);
    connect(Core(), &CutterCore::refreshAll, this, &DiffGraphView::refreshView);
    connect(cutterDiff, &CutterDiff::currentItemDiffChanged, this, &DiffGraphView::refreshView);
    connect(actionCenter, &QAction::triggered, this, [this]() {
        center();
        this->viewport()->update();
    });
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addAction(actionCenter);
    contextMenu->addMenu(layoutMenu);
}

DiffGraphView::~DiffGraphView() {}

void DiffGraphView::refreshView()
{
    CutterGraphView::refreshView();
    // loadCurrentGraph(diffGraphMode);
    viewport()->update();
    emit viewRefreshed();
}

ut64 DiffGraphView::graphEntryFromOffset(ut64 offset, bool original)
{
    if (original) {
        return blocksA.contains(offset) ? blocksA.at(offset) : RVA_INVALID;
    }
    return blocksB.contains(offset) ? blocksB.at(offset) : RVA_INVALID;
}

void DiffGraphView::drawDiffLine(QPainter &p, const QString &instr, int x, int y,
                                 DiffInstrType type)
{
    QColor background;
    if (instr.trimmed().isEmpty()) {
        return;
    }
    switch (type) {
    case DiffInstrEqual:
        background = Qt::transparent;
        break;
    case DiffInstrInserted:
        background = Config()->getColor("gui.match.perfect");
        background.setAlpha(50);
        break;
    case DiffInstrDeleted:
        background = Config()->getColor("gui.match.partial");
        background.setAlpha(50);
        break;
    default:
        background = Qt::transparent;
        break;
    }
    const int textWidth = mFontMetrics->width(instr);

    const QRect textRect(x, y - charHeight, textWidth + charWidthA, charHeight);
    p.fillRect(textRect, background);
    p.setPen(QPen(palette().color(QPalette::Text), 1));
    ;
    p.drawText(QPoint(x, y), instr);
}

void DiffGraphView::addDiffGraphBlockMatched(const CutterDiffItem &diffItem)
{
    if (!diffItem.isBlock()) {
        return;
    }
    if (diffItem.getType() != DiffItemMatched) {
        return;
    }
    // convert offsets to entries: Revise there is change of inefficient use of indices which may
    // increase exponentially
    ut64 entry = graphEntryFromOffset(diffItem.descriptionA()["offset"].toULongLong(), true);
    const RVA bbiFailA = diffItem.descriptionA()["fail"].toULongLong();
    const RVA bbiJumpA = diffItem.descriptionA()["jump"].toULongLong();
    const RVA bbiFailB = diffItem.descriptionB()["fail"].toULongLong();
    const RVA bbiJumpB = diffItem.descriptionB()["jump"].toULongLong();
    DiffBlock db;
    GraphBlock gb;
    gb.entry = entry;
    db.entry = entry;
    if (Config()->getGraphBlockEntryOffset()) {
        db.headerText =
                QString("[%0]->\n[%1]\nSimilarity[%2]")
                        .arg(rzAddressString(diffItem.descriptionA()["offset"].toULongLong()))
                        .arg(rzAddressString(diffItem.descriptionB()["offset"].toULongLong()))
                        .arg(diffItem.getSimilarity());
    }
    db.truePathA = RVA_INVALID;
    db.falsePathA = RVA_INVALID;
    db.truePathB = RVA_INVALID;
    db.falsePathB = RVA_INVALID;
    db.type = diffItem.getType();
    if (bbiFailA != RVA_INVALID) {
        db.falsePathA = graphEntryFromOffset(bbiFailA, true);
        if (diffGraphMode != Modified) {
            gb.edges.emplace_back(graphEntryFromOffset(bbiFailA, true));
        }
    }
    if (bbiFailB != RVA_INVALID) {
        db.falsePathB = graphEntryFromOffset(bbiFailB, false);
        if (diffGraphMode != Original) {
            gb.edges.emplace_back(graphEntryFromOffset(bbiFailB, false));
        }
    }
    if (bbiJumpA != RVA_INVALID) {
        if (bbiFailA != RVA_INVALID) {
            db.truePathA = graphEntryFromOffset(bbiJumpA, true);
        }
        if (diffGraphMode != Modified) {
            gb.edges.emplace_back(graphEntryFromOffset(bbiJumpA, true));
        }
    }
    if (bbiJumpB != RVA_INVALID) {
        if (bbiFailB != RVA_INVALID) {
            db.truePathB = graphEntryFromOffset(bbiJumpB, false);
        }
        if (diffGraphMode != Original) {
            gb.edges.emplace_back(graphEntryFromOffset(bbiJumpB, false));
        }
    }
    if (diffItem.descriptionA().contains("casejumps")) {
        for (const qulonglong jump :
             diffItem.descriptionA()["casejumps"].value<QList<qulonglong>>()) {
            ut64 graphEntry = graphEntryFromOffset(jump, true);
            db.caseOps[graphEntry] = DiffItemRemoved;
        }
    }
    if (diffItem.descriptionB().contains("casejumps")) {
        for (const qulonglong jump :
             diffItem.descriptionB()["casejumps"].value<QList<qulonglong>>()) {
            ut64 graphEntry = graphEntryFromOffset(jump, false);
            if (db.caseOps.contains(graphEntry)) {
                db.caseOps[graphEntry] = DiffItemMatched;
            } else {
                db.caseOps[graphEntry] = DiffItemAdded;
            }
        }
    }

    for (auto it = db.caseOps.cbegin(); it != db.caseOps.cend(); ++it) {
        if (it.key() == RVA_INVALID) {
            continue;
        }
        gb.edges.emplace_back(it.key());
    }

    if (diffItem.getInstrDiffs().contains("disas")) {
        db.instrs = diffItem.getInstrDiffs()["disas"]; // Maybe reference it instead for avoiding
                                                       // space complexity issues
    }

    diffBlocks[db.entry] = db;
    prepareGraphNode(gb);
    addBlock(gb);
}

void DiffGraphView::addDiffGraphBlockMismatch(const CutterDiffItem &diffItem)
{
    if (!diffItem.isBlock()) {
        return;
    }
    if (diffItem.getType() == DiffItemMatched) {
        return;
    }
    const bool isOriginal = diffItem.getType() == DiffItemRemoved;
    // convert offsets to entries: Revise there is change of inefficient use of indices which may
    // increase exponentially
    const CutterDiffItemDescription &desc = diffItem.getType() == DiffItemRemoved
            ? diffItem.descriptionA()
            : diffItem.descriptionB();
    const ut64 entry = graphEntryFromOffset(desc["offset"].toULongLong(), isOriginal);
    const RVA bbiFail = desc["fail"].toULongLong();
    const RVA bbiJump = desc["jump"].toULongLong();
    DiffBlock db;
    GraphBlock gb;
    gb.entry = entry;
    db.entry = entry;
    if (Config()->getGraphBlockEntryOffset()) {
        db.headerText = QString("[%0]").arg(rzAddressString(desc["offset"].toULongLong()));
    }
    db.truePathA = RVA_INVALID;
    db.falsePathA = RVA_INVALID;
    db.truePathB = RVA_INVALID;
    db.falsePathB = RVA_INVALID;
    db.type = diffItem.getType();
    if (bbiFail != RVA_INVALID) {
        if (isOriginal) {
            db.falsePathA = graphEntryFromOffset(bbiFail, true);
        } else {
            db.falsePathB = graphEntryFromOffset(bbiFail, false);
        }
        gb.edges.emplace_back(graphEntryFromOffset(bbiFail, isOriginal));
    }
    if (bbiJump != RVA_INVALID) {
        if (bbiFail != RVA_INVALID) {
            if (isOriginal) {
                db.truePathA = graphEntryFromOffset(bbiJump, true);
            } else {
                db.truePathB = graphEntryFromOffset(bbiJump, false);
            }
        }
        gb.edges.emplace_back(graphEntryFromOffset(bbiJump, isOriginal));
    }

    if (desc.contains("casejumps")) {
        for (const qulonglong jump : desc["casejumps"].value<QList<qulonglong>>()) {
            const ut64 graphEntry = graphEntryFromOffset(jump, isOriginal);
            db.caseOps[graphEntry] = diffItem.getType();
        }
    }
    if (desc.contains("disas")) {
        DiffInstr instr;
        if (isOriginal) {
            instr.a = desc["disas"].toString();
        } else {
            instr.b = desc["disas"].toString();
        }
        instr.type = isOriginal ? DiffInstrDeleted : DiffInstrInserted;
        db.instrs.emplace_back(instr);
    }
    diffBlocks[db.entry] = db;
    prepareGraphNode(gb);
    addBlock(gb);
}

void DiffGraphView::loadCurrentGraph(DiffGraphMode graphMode)
{
    diffBlocks.clear();
    blocks.clear();
    blocksA.clear();
    blocksB.clear();

    diffGraphMode = graphMode;

    // map offsets to graphblock entries
    // RVA_INVALID or 0 can get mapped incase of missing entries
    ut64 count = 0;
    for (const CutterDiffItem &diffBlock : cutterDiff->getCurrentDiffItem().getBlocks()) {
        switch (diffBlock.getType()) {
        case DiffItemMatched:
            blocksA[diffBlock.descriptionA()["offset"].toULongLong()] = count;
            blocksB[diffBlock.descriptionB()["offset"].toULongLong()] = count++;
            break;
        case DiffItemAdded:
            blocksB[diffBlock.descriptionB()["offset"].toULongLong()] = count++;
            break;
        case DiffItemRemoved:
            blocksA[diffBlock.descriptionA()["offset"].toULongLong()] = count++;
            break;
        default:
            break;
        }
    }

    for (const CutterDiffItem &diffBlock : cutterDiff->getCurrentDiffItem().getBlocks()) {
        if (diffBlock.getType() == DiffItemType::DiffItemMatched) {
            addDiffGraphBlockMatched(diffBlock);
        } else if (diffBlock.getType() == DiffItemType::DiffItemAdded && (graphMode != Original)) {
            addDiffGraphBlockMismatch(diffBlock);
        } else if (diffBlock.getType() == DiffItemType::DiffItemRemoved
                   && (graphMode != Modified)) { // Block Removed
            addDiffGraphBlockMismatch(diffBlock);
        }
    }

    cleanupEdges(blocks);
    computeGraphPlacement();
}

void DiffGraphView::prepareGraphNode(GraphBlock &block)
{
    DiffBlock &db = diffBlocks[block.entry];
    auto lineCount = [](const QString &str) { return str.isEmpty() ? 0 : str.count('\n') + 1; };

    auto longestLine = [this](const QString &str) {
        double longest = 0;

        for (const QString &line : str.split('\n')) {
            longest = qMax(longest, mFontMetrics->width(line));
        }

        return longest;
    };

    double width = longestLine(db.headerText);
    double height = 1 + lineCount(db.headerText);
    auto trimRight = [](QString &str) {
        while (!str.isEmpty() && str.back().isSpace()) {
            str.chop(1);
        }
    };

    for (DiffInstr &instr : db.instrs) {
        trimRight(instr.a);
        trimRight(instr.b);
        if (db.type == DiffItemMatched) {
            width = qMax(width, longestLine(instr.a));
            width = qMax(width, longestLine(instr.b));
        } else if (db.type == DiffItemAdded) {
            width = qMax(width, longestLine(instr.b));
        } else {
            width = qMax(width, longestLine(instr.a));
        }

        if (diffGraphMode == Unified) {
            height += lineCount(instr.a) + lineCount(instr.b);
        } else {
            height += qMax(lineCount(instr.a), lineCount(instr.b));
        }
    }

    const double extra = static_cast<int>(2 * padding + 4);
    const double indent = charWidthA;

    block.width = width + extra + indent;
    block.height = height * charHeight + extra;
}

void DiffGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool)
{
    QColor matched = Config()->getColor("gui.match.perfect");
    QColor unmatched = Config()->getColor("gui.match.partial");
    matched.setAlpha(50);
    unmatched.setAlpha(50);
    const QRectF blockRect(block.x, block.y, block.width, block.height);
    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    const DiffBlock &db = diffBlocks[block.entry];
    const bool blockSelected =
            currentBlockEntry == block.entry; // use it later when implementing navigation
    p.setPen(QColor(0, 0, 0, 0));
    if (db.terminal) { // coudn't find where does it set terminal not in the loadgraphfunction
        p.setBrush(retShadowColor);
    } else if (db.indirectcall) {
        p.setBrush(indirectcallShadowColor);
    } else {
        p.setBrush(QColor(0, 0, 0, 100));
    }

    // Adding similarity score and offsets of both of the blocks to the graph diagram would be nice

    p.setPen(QPen(graphNodeColor, 1));
    p.setBrush(disassemblyBackgroundColor);
    if (blockSelected) {
        if (db.type == DiffItemRemoved) {
            p.setBrush(QColor(unmatched.red(), unmatched.green(), unmatched.blue(), 100));
        } else if (db.type == DiffItemAdded) {
            p.setBrush(QColor(matched.red(), matched.green(), matched.blue(), 100));
        } else {
            p.setBrush(disassemblyBackgroundColor);
        }
    } else {
        if (db.type == DiffItemRemoved) {
            p.setBrush(unmatched);
        } else if (db.type == DiffItemAdded) {
            p.setBrush(matched);
        } else {
            p.setBrush(disassemblyBackgroundColor);
        }
    }

    p.drawRect(blockRect);

    auto transform = p.combinedTransform();
    const QRect screenChar = transform.mapRect(QRect(0, 0, charWidthA, charHeight));
    if (screenChar.width() < Config()->getGraphMinFontSize()) {
        return;
    }
    // const qreal indent = charWidthA;

    auto x = block.x + padding;
    int y = block.y + getTextOffset(0).y();
    // for (auto &line : db.headerText.lines) {
    //     RichTextPainter::paintRichText<qreal>(&p, x, y, block.width, charHeight, 0, line,
    //                                           mFontMetrics.get());
    //     y += charHeight;
    // }
    y += charHeight;
    for (const QString &line : db.headerText.split("\n")) {
        p.drawText(QPointF(x, y), line);
        y += charHeight;
    }
    auto lineCount = [](const QString &str) { return str.isEmpty() ? 0 : str.count('\n') + 1; };
    if (db.type == DiffItemMatched) {
        if (diffGraphMode == Unified) {
            for (const DiffInstr &instr : db.instrs) {
                p.setPen(QPen(palette().color(QPalette::Text), 1));
                if (instr.type == DiffInstrEqual) {
                    for (const QString &line : instr.a.split("\n")) {
                        // color for Equal
                        drawDiffLine(p, line, x, y);
                        y += charHeight;
                    }
                } else if (instr.type == DiffInstrInserted) {
                    for (const QString &line : instr.b.split("\n")) {
                        // color for Added
                        drawDiffLine(p, line, x, y, DiffInstrInserted);
                        y += charHeight;
                    }
                } else if (instr.type == DiffInstrDeleted) {
                    for (const QString &line : instr.a.split("\n")) {
                        // color for Removed
                        drawDiffLine(p, line, x, y, DiffInstrDeleted);
                        y += charHeight;
                    }
                } else {
                    for (const QString &line : instr.a.split("\n")) {
                        // color for Removed
                        p.drawText(QPoint(x, y), line);
                        drawDiffLine(p, line, x, y, DiffInstrDeleted);
                        y += charHeight;
                    }
                    for (const QString &line : instr.b.split("\n")) {
                        // color for Added
                        p.drawText(QPoint(x, y), line);
                        drawDiffLine(p, line, x, y, DiffInstrInserted);
                        y += charHeight;
                    }
                }
            }
        } else {
            for (const DiffInstr &instr : db.instrs) {
                p.setPen(QPen(palette().color(QPalette::Text), 1));
                const int aLines = lineCount(instr.a);
                const int bLines = lineCount(instr.b);
                const int slotHeight = qMax(aLines, bLines);

                if (diffGraphMode == Original) {
                    QStringList instrList;

                    if (instr.type == DiffInstrEqual || instr.type == DiffInstrDeleted
                        || instr.type == DiffInstrReplaced) {

                        instrList = instr.a.isEmpty() ? QStringList {} : instr.a.split('\n');

                        for (const QString &line : instrList) {
                            drawDiffLine(p, line, x, y,
                                         instr.type == DiffInstrEqual ? DiffInstrEqual
                                                                      : DiffInstrDeleted);
                            y += charHeight;
                        }
                    }

                    const int displayedLines = instrList.size();
                    y += (slotHeight - displayedLines) * charHeight;
                } else if (diffGraphMode == Modified) {
                    QStringList instrList;

                    if (instr.type == DiffInstrEqual) {
                        instrList = instr.a.isEmpty() ? QStringList {} : instr.a.split('\n');
                    } else if (instr.type == DiffInstrInserted || instr.type == DiffInstrReplaced) {
                        instrList = instr.b.isEmpty() ? QStringList {} : instr.b.split('\n');
                    }

                    for (const QString &line : instrList) {
                        drawDiffLine(p, line, x, y,
                                     instr.type == DiffInstrEqual ? DiffInstrEqual
                                                                  : DiffInstrInserted);
                        y += charHeight;
                    }

                    const int displayedLines = instrList.size();
                    y += (slotHeight - displayedLines) * charHeight;
                }
            }
        }
    } else {
        for (const DiffInstr &instr : db.instrs) {
            p.setPen(QPen(palette().color(QPalette::Text), 1));
            const QStringList &instrList =
                    db.type == DiffItemRemoved ? instr.a.split("\n") : instr.b.split("\n");
            for (const QString &line : instrList) {
                // if condition for background color of added/removed text
                p.drawText(QPoint(x, y), line);
                y += charHeight;
            }
        }
    }
}

GraphView::EdgeConfiguration DiffGraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                              GraphView::GraphBlock *to,
                                                              bool interactive)
{
    EdgeConfiguration ec;
    const DiffBlock &db = diffBlocks[from.entry];

    // Original arrows are showed as dotted lines new ones are solid lines

    if (to->entry == db.truePathA) {
        if (db.truePathA != db.truePathB) {
            ec.color = brtrueColor;
            ec.lineStyle = Qt::DotLine;
        } else {
            ec.color = brtrueColor;
            ec.lineStyle = Qt::SolidLine;
        }
    } else if (to->entry == db.falsePathA) {
        if (db.falsePathA != db.falsePathB) {
            ec.color = brfalseColor;
            ec.lineStyle = Qt::DotLine;
        } else {
            ec.color = brfalseColor;
            ec.lineStyle = Qt::SolidLine;
        }
    } else if (to->entry == db.truePathB) {
        ec.color = brtrueColor;
    } else if (to->entry == db.falsePathB) {
        ec.color = brfalseColor;
    } else {
        ec.color = jmpColor;
        if (db.caseOps[to->entry] == DiffItemRemoved) {
            ec.lineStyle = Qt::DotLine; // show removed lines as dotted
        }
    } // two new types of edges for old and new for unified view

    ec.startArrow = false;
    ec.endArrow = true;
    if (interactive) {
        if (interactive) {
            if (from.entry == currentBlockEntry) {
                ec.widthScale = 2.0;
            } else if (to->entry == currentBlockEntry) {
                ec.widthScale = 2.0;
            }
        }
    }
    return ec;
}

bool DiffGraphView::eventFilter(QObject *obj, QEvent *event)
{
    // Would be needed in future
    // in Disassembler graphView it is mainly used for showing tooltips
    return CutterGraphView::eventFilter(obj, event);
}

void DiffGraphView::keyPressEvent(QKeyEvent *event)
{
    // mostly for navigating between the graph nodes only Qt::Key_Return is mapped
    // don't thing this is needed for diffGraphview will be refering
    CutterGraphView::keyPressEvent(event);
}

void DiffGraphView::setTooltipStylesheet()
{
    // not needed since not planned for showing tooltips
    setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}

void DiffGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos)
{
    // Mostly instruction specific logic
    currentBlockEntry = block.entry;
    viewport()->update();
}

void DiffGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                              QContextMenuEvent *event, QPoint pos)
{
    // display context menu for the given block with appropriate options
    GraphView::blockContextMenuRequested(block, event, pos);
}

void DiffGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void DiffGraphView::showExportDialog()
{
    QString defaultName;
    const CutterDiffItem &diffItem = cutterDiff->getCurrentDiffItem();
    const DiffItemType type = diffItem.getType();

    const uint64_t bitmapExportWarningSize = 32 * 1024 * 1024;

    // May need to match default conventional naming of graphs
    if (type == DiffItemMatched) {
        defaultName = QString("%0->%1")
                              .arg(diffItem.descriptionA()["name"].toString())
                              .arg(diffItem.descriptionB()["name"].toString());
    } else if (type == DiffItemAdded) {
        defaultName = QString("%0").arg(diffItem.descriptionA()["name"].toString());
    } else {
        defaultName = QString("%0").arg(diffItem.descriptionB()["name"].toString());
    }

    defaultName.replace(QRegularExpression("[.:]"), "_");
    defaultName.remove(QRegularExpression("[^a-zA-Z0-9_].*"));
    if (defaultName.isEmpty()) {
        defaultName = "graph";
    }
    const QVector<MultitypeFileSaveDialog::TypeDescription> types = {
        { tr("PNG (*.png)"), "png", QVariant::fromValue(GraphExportType::Png) },
        { tr("JPEG (*.jpg)"), "jpg", QVariant::fromValue(GraphExportType::Jpeg) },
        { tr("SVG (*.svg)"), "svg", QVariant::fromValue(GraphExportType::Svg) }
    };

    MultitypeFileSaveDialog dialog(this, tr("Export Graph"));
    dialog.setTypes(types);
    dialog.selectFile(defaultName);
    if (!dialog.exec()) {
        return;
    }

    auto selectedType = dialog.selectedType();
    if (!selectedType.data.canConvert<GraphExportType>()) {
        qWarning() << "Bad selected type, should not happen.";
        return;
    }
    auto exportType = selectedType.data.value<GraphExportType>();

    if (graphIsBitamp(exportType)) {
        const uint64_t bitmapSize = uint64_t(width) * uint64_t(height);
        if (bitmapSize > bitmapExportWarningSize) {
            auto answer =
                    QMessageBox::question(this, tr("Graph Export"),
                                          tr("Do you really want to export %1 x %2 = %3 pixel "
                                             "bitmap image? Consider using different format.")
                                                  .arg(width)
                                                  .arg(height)
                                                  .arg(bitmapSize));
            if (answer != QMessageBox::Yes) {
                return;
            }
        }
    }

    const QString filePath = dialog.selectedFiles().first();
    const bool graphTransparent = Config()->getBitmapTransparentState();
    const double graphScaleFactor = Config()->getBitmapExportScaleFactor();
    switch (exportType) {
    case GraphExportType::Png:
        this->saveAsBitmap(filePath, "png", graphScaleFactor, graphTransparent);
        break;
    case GraphExportType::Jpeg:
        this->saveAsBitmap(filePath, "jpg", graphScaleFactor, false);
        break;
    case GraphExportType::Svg:
        this->saveAsSvg(filePath);
        break;
    default:
        qInfo() << "Export format not supported yet.";
        break;
    }
}

void DiffGraphView::blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos)
{
}

void DiffGraphView::paintEvent(QPaintEvent *event)
{
    // DisassemblerGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}
