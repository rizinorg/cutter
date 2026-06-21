#include "DisassemblerGraphView.h"

#include "common/BasicBlockHighlighter.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/Configuration.h"
#include "common/CutterSeekable.h"
#include "common/DisassemblyHelper.h"
#include "common/DisassemblyPreview.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QShortcut>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>

namespace DH = DisassemblyHelper;

DisassemblerGraphView::DisassemblerGraphView(QWidget *parent, CutterSeekable *seekable,
                                             MainWindow *mainWindow,
                                             const QList<QAction *> &additionalMenuActions)
    : CutterGraphView(parent),
      highlightToken(nullptr),
      blockMenu(new DisassemblyContextMenu(this, mainWindow)),
      contextMenu(new QMenu(this)),
      seekable(seekable),
      actionUnhighlight(this),
      actionUnhighlightInstruction(this)
{
    auto *layout = new QVBoxLayout(this);
    setTooltipStylesheet();

    // Signals that require a refresh all
    connect(Config(), &Configuration::colorsUpdated, this,
            &DisassemblerGraphView::setTooltipStylesheet);

    connect(Core(), &CutterCore::refreshAll, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::commentsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::functionRenamed, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::flagsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::globalVarsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::varsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::instructionChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::breakpointsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::functionsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::asmOptionsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::refreshCodeViews, this, &DisassemblerGraphView::refreshView);

    connectSeekChanged(false);

    // ESC for previous
    QShortcut *shortcutEscape = Shortcuts()->makeQShortcut("General.seekPrev", this);
    shortcutEscape->setContext(Qt::WidgetShortcut);
    connect(shortcutEscape, &QShortcut::activated, seekable, &CutterSeekable::seekPrev);

    // Branch shortcuts
    QShortcut *shortcutTakeTrue = Shortcuts()->makeQShortcut("Graph.takeTrue", this);
    shortcutTakeTrue->setContext(Qt::WidgetShortcut);
    connect(shortcutTakeTrue, &QShortcut::activated, this, &DisassemblerGraphView::takeTrue);
    QShortcut *shortcutTakeFalse = Shortcuts()->makeQShortcut("Graph.takeFalse", this);
    shortcutTakeFalse->setContext(Qt::WidgetShortcut);
    connect(shortcutTakeFalse, &QShortcut::activated, this, &DisassemblerGraphView::takeFalse);

    // Navigation shortcuts
    QShortcut *shortcutNextInstr = Shortcuts()->makeQShortcut("Graph.nextInstr", this);
    shortcutNextInstr->setContext(Qt::WidgetShortcut);
    connect(shortcutNextInstr, &QShortcut::activated, this, &DisassemblerGraphView::nextInstr);
    QShortcut *shortcutPrevInstr = Shortcuts()->makeQShortcut("Graph.prevInstr", this);
    shortcutPrevInstr->setContext(Qt::WidgetShortcut);
    connect(shortcutPrevInstr, &QShortcut::activated, this, &DisassemblerGraphView::prevInstr);
    shortcuts.append(shortcutEscape);
    shortcuts.append(shortcutNextInstr);
    shortcuts.append(shortcutPrevInstr);

    // Context menu that applies to everything
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addMenu(layoutMenu);
    contextMenu->addSeparator();
    contextMenu->addActions(additionalMenuActions);

    auto *highlightBB = new QAction(this);
    actionUnhighlight.setVisible(false);

    highlightBB->setText(tr("Highlight block"));
    connect(highlightBB, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        const RVA currBlockEntry = blockForAddress(this->seekable->getOffset())->entry;

        QColor background = disassemblyBackgroundColor;
        if (auto block = bbh->getBasicBlock(currBlockEntry)) {
            background = block->color;
        }

        const QColor c = QColorDialog::getColor(background, this, QString(),
                                                QColorDialog::DontUseNativeDialog);
        if (c.isValid()) {
            bbh->highlight(currBlockEntry, c);
        }
        emit Config()->colorsUpdated();
    });

    actionUnhighlight.setText(tr("Unhighlight block"));
    connect(&actionUnhighlight, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        bbh->clear(blockForAddress(this->seekable->getOffset())->entry);
        emit Config()->colorsUpdated();
    });

    auto *highlightBI = new QAction(this);
    actionUnhighlightInstruction.setVisible(false);

    highlightBI->setText(tr("Highlight instruction"));
    connect(highlightBI, &QAction::triggered, this,
            &DisassemblerGraphView::onActionHighlightBITriggered);

    actionUnhighlightInstruction.setText(tr("Unhighlight instruction"));
    connect(&actionUnhighlightInstruction, &QAction::triggered, this,
            &DisassemblerGraphView::onActionUnhighlightBITriggered);

    blockMenu->addAction(highlightBB);
    blockMenu->addAction(&actionUnhighlight);
    blockMenu->addAction(highlightBI);
    blockMenu->addAction(&actionUnhighlightInstruction);

    // Include all actions from generic context menu in block specific menu
    blockMenu->addSeparator();
    blockMenu->addActions(contextMenu->actions());

    this->addActions(blockMenu->actions());

    connect(blockMenu, &DisassemblyContextMenu::copy, this, &DisassemblerGraphView::copySelection);

    // Add header as widget to layout so it stretches to the layout width
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    this->scaleThicknessMultiplier = true;
    installEventFilter(this);
}

void DisassemblerGraphView::connectSeekChanged(bool disconn)
{
    if (disconn) {
        disconnect(seekable, &CutterSeekable::seekableSeekChanged, this,
                   &DisassemblerGraphView::onSeekChanged);
    } else {
        connect(seekable, &CutterSeekable::seekableSeekChanged, this,
                &DisassemblerGraphView::onSeekChanged);
    }
}

DisassemblerGraphView::~DisassemblerGraphView()
{
    qDeleteAll(shortcuts);
    shortcuts.clear();
    delete highlightToken;
}

void DisassemblerGraphView::refreshView()
{
    CutterGraphView::refreshView();
    loadCurrentGraph();
    breakpoints = Core()->getBreakpointsAddresses();
    emit viewRefreshed();
}

void DisassemblerGraphView::loadCurrentGraph()
{
    TempConfig tempConfig;
    tempConfig.set("scr.color", COLOR_MODE_16M)
            .set("asm.bb.line", false)
            .set("asm.lines", false)
            .set("asm.lines.fcn", false);

    disassemblyBlocks.clear();
    blocks.clear();

    if (highlightToken) {
        delete highlightToken;
        highlightToken = nullptr;
    }

    const RzAnalysisFunction *fcn = Core()->functionIn(seekable->getOffset());

    windowTitle = tr("Graph");
    if (fcn && RZ_STR_ISNOTEMPTY(fcn->name)) {
        currentFcnAddr = fcn->addr;
        auto fcnName = fromOwned(rz_str_escape_utf8_for_json(fcn->name, -1));
        windowTitle += QString("(%0)").arg(fcnName.get());
    } else {
        windowTitle += tr("(Empty)");
    }
    emit nameChanged(windowTitle);

    emptyGraph = !fcn;
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
    setEntry(fcn ? fcn->addr : RVA_INVALID);

    if (!fcn) {
        return;
    }

    for (const auto &bbi : CutterPVector<RzAnalysisBlock>(fcn->bbs)) {
        const RVA bbiFail = bbi->fail;
        const RVA bbiJump = bbi->jump;

        DisassemblyBlock db;
        GraphBlock gb;
        gb.entry = bbi->addr;
        db.entry = bbi->addr;
        if (Config()->getGraphBlockEntryOffset()) {
            // QColor(0,0,0,0) is transparent
            db.headerText = Text("[" + rzAddressString(db.entry) + "]", ConfigColor("offset"),
                                 QColor(0, 0, 0, 0));
        }
        db.truePath = RVA_INVALID;
        db.falsePath = RVA_INVALID;
        if (bbiFail) {
            db.falsePath = bbiFail;
            gb.edges.emplace_back(bbiFail);
        }
        if (bbiJump) {
            if (bbiFail) {
                db.truePath = bbiJump;
            }
            gb.edges.emplace_back(bbiJump);
        }

        const RzAnalysisSwitchOp *switchOp = bbi->switch_op;
        if (switchOp) {
            for (const auto &caseOp : CutterRzList<RzAnalysisCaseOp>(switchOp->cases)) {
                if (caseOp->jump == RVA_INVALID) {
                    continue;
                }
                gb.edges.emplace_back(caseOp->jump);
            }
        }

        RzCoreLocked core(Core());
        const std::unique_ptr<ut8[]> buf { new ut8[bbi->size] };
        if (!buf) {
            break;
        }
        rz_io_read_at_mapped(core->io, bbi->addr, buf.get(), (int)bbi->size);

        auto vec = fromOwned(
                rz_pvector_new(reinterpret_cast<RzPVectorFree>(rz_analysis_disasm_text_free)));
        if (!vec) {
            break;
        }

        RzCoreDisasmOptions options = {};
        options.vec = vec.get();
        options.cbytes = 1;

        rz_core_print_disasm(core, bbi->addr, buf.get(), (int)bbi->size, (int)bbi->size, nullptr,
                             &options);

        auto vecVisitor = CutterPVector<RzAnalysisDisasmText>(vec.get());
        auto iter = vecVisitor.begin();
        while (iter != vecVisitor.end()) {
            const RzAnalysisDisasmText *op = *iter;
            Instr instr;
            instr.addr = op->offset;

            ++iter;
            if (iter != vecVisitor.end()) {
                // get instruction size from distance to next instruction ...
                const RVA nextOffset = (*iter)->offset;
                instr.size = nextOffset - instr.addr;
            } else {
                // or to the end of the block.
                instr.size = (bbi->addr + bbi->size) - instr.addr;
            }

            QTextDocument textDoc;
            textDoc.setHtml(CutterCore::ansiEscapeToHtml(op->text));

            instr.plainText = textDoc.toPlainText();

            const RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);
            // Colors::colorizeAssembly(richText, textDoc.toPlainText(), 0);

            bool cropped;
            const int blockLength = Config()->getGraphBlockMaxChars()
                    + Core()->getConfigb("asm.bytes") * 24 + Core()->getConfigb("asm.emu") * 10;
            instr.text = Text(RichTextPainter::cropped(richText, blockLength, "...", &cropped));
            if (cropped) {
                instr.fullText = richText;
            } else {
                instr.fullText = Text();
            }
            db.instrs.push_back(instr);
        }
        disassemblyBlocks[db.entry] = db;
        prepareGraphNode(gb);
        addBlock(gb);
    }
    cleanupEdges(blocks);
    computeGraphPlacement();
}

DisassemblerGraphView::EdgeConfigurationMapping DisassemblerGraphView::getEdgeConfigurations()
{
    EdgeConfigurationMapping result;
    for (auto &block : blocks) {
        for (const auto &edge : block.second.edges) {
            result[{ block.first, edge.target }] =
                    edgeConfiguration(block.second, &blocks[edge.target], false);
        }
    }
    return result;
}

void DisassemblerGraphView::prepareGraphNode(GraphBlock &block)
{
    const DisassemblyBlock &db = disassemblyBlocks[block.entry];
    int width = 0;
    int height = 0;
    for (auto &line : db.headerText.lines) {
        int lw = 0;
        for (auto &part : line) {
            lw += mFontMetrics->width(part.text);
        }
        if (lw > width) {
            width = lw;
        }
        height += 1;
    }
    for (const Instr &instr : db.instrs) {
        for (auto &line : instr.text.lines) {
            int lw = 0;
            for (auto &part : line) {
                lw += mFontMetrics->width(part.text);
            }
            if (lw > width) {
                width = lw;
            }
            height += 1;
        }
    }
    const int extra = static_cast<int>(2 * padding + 4);
    const qreal indent = charWidthA;
    block.width = static_cast<int>(width + extra + indent);
    block.height = (height * charHeight) + extra;
}

void DisassemblerGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    const QRectF blockRect(block.x, block.y, block.width, block.height);

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    // Render node
    const DisassemblyBlock &db = disassemblyBlocks[block.entry];
    bool blockSelected = false;
    RVA selectedInstruction = RVA_INVALID;

    // Figure out if the current block is selected
    const RVA addr = seekable->getOffset();
    const RVA pcAddr = Core()->getProgramCounterValue();
    for (const Instr &instr : db.instrs) {
        if (instr.contains(addr) && interactive) {
            blockSelected = true;
            selectedInstruction = instr.addr;
        }

        // TODO: L219
    }

    p.setPen(QColor(0, 0, 0, 0));
    if (db.terminal) {
        p.setBrush(retShadowColor);
    } else if (db.indirectcall) {
        p.setBrush(indirectcallShadowColor);
    } else {
        p.setBrush(QColor(0, 0, 0, 100));
    }

    p.setPen(QPen(graphNodeColor, 1));

    if (blockSelected) {
        p.setBrush(disassemblySelectedBackgroundColor);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }

    // Draw basic block background
    p.drawRect(blockRect);
    auto bb = Core()->getBBHighlighter()->getBasicBlock(block.entry);
    if (bb) {
        const QColor color(bb->color);
        p.setBrush(color);
        p.drawRect(blockRect);
    }

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    const QRect screenChar = transform.mapRect(QRect(0, 0, charWidthA, charHeight));

    if (screenChar.width() < Config()->getGraphMinFontSize()) {
        return;
    }

    const qreal indent = charWidthA;

    // Render node text
    auto x = block.x + padding;
    int y = block.y + getTextOffset(0).y();
    for (auto &line : db.headerText.lines) {
        RichTextPainter::paintRichText<qreal>(&p, x, y, block.width, charHeight, 0, line,
                                              mFontMetrics.get());
        y += charHeight;
    }

    auto bih = Core()->getBIHighlighter();
    const QColor selectionColor = ConfigColor("wordHighlight");

    for (const Instr &instr : db.instrs) {
        const QRect instrRect = QRect(static_cast<int>(block.x + indent), y,
                                      static_cast<int>(block.width - (10 + padding)),
                                      int(instr.text.lines.size()) * charHeight);

        QColor instrColor;
        if (Core()->isBreakpoint(breakpoints, instr.addr)) {
            instrColor = ConfigColor("gui.breakpoint_background");
        } else if (instr.addr == pcAddr) {
            instrColor = pcSelectionColor;
        } else if (auto background = bih->getBasicInstruction(instr.addr)) {
            instrColor = background->color;
        }

        if (instrColor.isValid()) {
            p.fillRect(instrRect, instrColor);
        }

        if (selectedInstruction != RVA_INVALID && selectedInstruction == instr.addr) {
            p.fillRect(instrRect, disassemblySelectionColor);
        }

        // Highlight selected tokens
        if (interactive && highlightToken != nullptr) {
            int pos = -1;
            const qreal tokenWidth = mFontMetrics->width(highlightToken->content);
            while ((pos = instr.plainText.indexOf(highlightToken->content, pos + 1)) != -1) {
                const int tokenEnd = pos + highlightToken->content.length();

                if ((pos > 0 && instr.plainText[pos - 1].isLetterOrNumber())
                    || (tokenEnd < instr.plainText.length()
                        && instr.plainText[tokenEnd].isLetterOrNumber())) {
                    continue;
                }

                const qreal widthBefore = mFontMetrics->width(instr.plainText.left(pos));
                const qreal textOffset = padding + indent;

                if (textOffset + widthBefore > block.width - (10 + padding)) {
                    continue;
                }

                qreal highlightWidth = tokenWidth;
                if (textOffset + widthBefore + tokenWidth >= block.width - (10 + padding)) {
                    highlightWidth = block.width - widthBefore - (10 + 2 * padding);
                }

                p.fillRect(
                        QRectF(block.x + textOffset + widthBefore, y, highlightWidth, charHeight),
                        selectionColor);
            }
        }

        for (auto &line : instr.text.lines) {
            RichTextPainter::paintRichText<qreal>(&p, x + indent, y, block.width - padding,
                                                  charHeight, 0, line, mFontMetrics.get());
            y += charHeight;
        }
    }
}

GraphView::EdgeConfiguration DisassemblerGraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                                      GraphView::GraphBlock *to,
                                                                      bool interactive)
{
    EdgeConfiguration ec;
    const DisassemblyBlock &db = disassemblyBlocks[from.entry];
    if (to->entry == db.truePath) {
        ec.color = brtrueColor;
    } else if (to->entry == db.falsePath) {
        ec.color = brfalseColor;
    } else {
        ec.color = jmpColor;
    }
    ec.startArrow = false;
    ec.endArrow = true;
    if (interactive) {
        if (from.entry == currentBlockAddress) {
            ec.widthScale = 2.0;
        } else if (to->entry == currentBlockAddress) {
            ec.widthScale = 2.0;
        }
    }
    return ec;
}

bool DisassemblerGraphView::eventFilter(QObject *obj, QEvent *event)
{
    if ((Config()->getGraphPreview() || Config()->getShowVarTooltips())
        && event->type() == QEvent::Type::ToolTip) {

        const auto *helpEvent = static_cast<QHelpEvent *>(event);
        const QPoint point = viewToLogicalCoordinates(helpEvent->pos());

        if (auto block = getBlockContaining(point)) {
            // Get pos relative to start of block
            QPoint pos = point - QPoint(block->x, block->y);

            // offsetFrom is the address which on top the cursor triggered this
            RVA offsetFrom = RVA_INVALID;

            /*
             * getAddrForMouseEvent() doesn't work for jmps, like
             * getInstrForMouseEvent() with false as a 3rd argument.
             */
            Instr *inst = getInstrForMouseEvent(*block, &pos, true);
            if (inst != nullptr) {
                offsetFrom = inst->addr;
            }

            // Don't preview anything for a small scale
            if (getViewScale() >= 0.8) {
                auto bracketValue = DH::findBracketRange(
                        inst->plainText, mFontMetrics->position(inst->plainText, pos.x()));

                DH::TargetContext ctx;
                ctx.offset = offsetFrom;
                if (bracketValue.found) {
                    ctx.word = bracketValue.content;
                } else if (auto token = getToken(inst, pos.x())) {
                    ctx.word = token->content;
                    delete token;
                } else {
                    ctx.word = QString();
                }
                ctx.line = inst->plainText;
                ctx.arrow = getTruePathForOffset(offsetFrom);
                if (DisassemblyPreview::showTooltip(this, helpEvent->globalPos(), ctx,
                                                    Config()->getGraphPreview())) {
                    return true;
                }
            }
        }
    }
    return CutterGraphView::eventFilter(obj, event);
}

void DisassemblerGraphView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return && seekable) {
        const RVA offset = seekable->getOffset();
        // pressing enter at last instruction of the block seeks to true path if valid
        const RVA truePath = getTruePathForOffset(offset);
        if (truePath != RVA_INVALID) {
            seekable->seek(truePath);
        } else {
            seekable->seekToReference(offset);
        }
    }

    CutterGraphView::keyPressEvent(event);
}

RVA DisassemblerGraphView::getAddrForMouseEvent(GraphBlock &block, QPoint *point)
{
    const DisassemblyBlock &db = disassemblyBlocks[block.entry];

    // Remove header and margin
    const int offY = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    const int textPointY = point->y() - offY;
    const int mouseRow = textPointY / charHeight;

    // If mouse coordinate is in header region or margin above
    if (mouseRow < 0) {
        return db.entry;
    }

    const Instr *instr = getInstrForMouseEvent(block, point);
    if (instr) {
        return instr->addr;
    }

    return RVA_INVALID;
}

DisassemblerGraphView::Instr *
DisassemblerGraphView::getInstrForMouseEvent(GraphView::GraphBlock &block, QPoint *point,
                                             bool force)
{
    DisassemblyBlock &db = disassemblyBlocks[block.entry];

    // Remove header and margin
    const int offY = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    const int textPointY = point->y() - offY;
    const int mouseRow = textPointY / charHeight;

    // Row in actual text
    int curRow = 0;

    for (Instr &instr : db.instrs) {
        if (mouseRow < curRow + (int)instr.text.lines.size()) {
            return &instr;
        }
        curRow += instr.text.lines.size();
    }
    if (force && !db.instrs.empty()) {
        if (mouseRow <= 0) {
            return &db.instrs.front();
        } else {
            return &db.instrs.back();
        }
    }

    return nullptr;
}

QRectF DisassemblerGraphView::getInstrRect(GraphView::GraphBlock &block, RVA addr) const
{
    auto blockIt = disassemblyBlocks.find(block.entry);
    if (blockIt == disassemblyBlocks.end()) {
        return QRectF();
    }
    auto &db = blockIt->second;
    if (db.instrs.empty()) {
        return QRectF();
    }

    size_t sequenceAddr = db.instrs[0].addr;
    size_t firstLineWithAddr = 0;
    size_t currentLine = 0;
    for (size_t i = 0; i < db.instrs.size(); i++) {
        auto &instr = db.instrs[i];
        if (instr.addr != sequenceAddr) {
            sequenceAddr = instr.addr;
            firstLineWithAddr = currentLine;
        }
        if (instr.contains(addr)) {
            while (i < db.instrs.size() && db.instrs[i].addr == sequenceAddr) {
                currentLine += db.instrs[i].text.lines.size();
                i++;
            }
            const QPointF topLeft = getInstructionOffset(db, static_cast<int>(firstLineWithAddr));
            return QRectF(topLeft,
                          QSizeF(block.width - 2 * padding,
                                 charHeight * int(currentLine - firstLineWithAddr)));
        }
        currentLine += instr.text.lines.size();
    }
    return QRectF();
}

RVA DisassemblerGraphView::getTruePathForOffset(RVA offset)
{
    DisassemblyBlock *db = blockForAddress(offset);
    if (db && !db->instrs.empty()) {
        const Instr lastInstruction = db->instrs.back();
        if (lastInstruction.addr == offset) {
            return db->truePath;
        }
    }
    return RVA_INVALID;
}

void DisassemblerGraphView::showInstruction(GraphView::GraphBlock &block, RVA addr)
{
    QRectF rect = getInstrRect(block, addr);
    rect.translate(block.x, block.y);
    showRectangle(QRect(rect.x(), rect.y(), rect.width(), rect.height()), true);
}

DisassemblerGraphView::DisassemblyBlock *DisassemblerGraphView::blockForAddress(RVA addr)
{
    for (auto &blockIt : disassemblyBlocks) {
        DisassemblyBlock &db = blockIt.second;
        for (const Instr &i : db.instrs) {
            if (i.addr == RVA_INVALID || i.size == RVA_INVALID) {
                continue;
            }

            if (i.contains(addr)) {
                return &db;
            }
        }
    }
    return nullptr;
}

const DisassemblerGraphView::Instr *DisassemblerGraphView::instrForAddress(RVA addr)
{
    const DisassemblyBlock *block = blockForAddress(addr);
    for (const Instr &i : block->instrs) {
        if (i.addr == RVA_INVALID || i.size == RVA_INVALID) {
            continue;
        }

        if (i.contains(addr)) {
            return &i;
        }
    }
    return nullptr;
}

void DisassemblerGraphView::onSeekChanged(RVA addr)
{
    blockMenu->setOffset(addr);
    const DisassemblyBlock *db = blockForAddress(addr);
    bool switchFunction = false;
    if (!db) {
        // not in this function, try refreshing
        refreshView();
        db = blockForAddress(addr);
        switchFunction = true;
    }
    if (db) {
        // This is a local address! We animated to it.
        transitionDontSeek = true;
        showBlock(blocks[db->entry], !switchFunction);
        showInstruction(blocks[db->entry], addr);
    }
}

void DisassemblerGraphView::takeTrue()
{
    const DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->truePath != RVA_INVALID) {
        seekable->seek(db->truePath);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void DisassemblerGraphView::takeFalse()
{
    const DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->falsePath != RVA_INVALID) {
        seekable->seek(db->falsePath);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void DisassemblerGraphView::setTooltipStylesheet()
{
    setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}

void DisassemblerGraphView::seekInstruction(bool previous_instr)
{
    const RVA addr = seekable->getOffset();
    DisassemblyBlock *db = blockForAddress(addr);
    if (!db) {
        return;
    }

    for (size_t i = 0; i < db->instrs.size(); i++) {
        const Instr &instr = db->instrs[i];
        if (!instr.contains(addr)) {
            continue;
        }

        // Found the instruction. Check if a next one exists
        if (!previous_instr && (i < db->instrs.size() - 1)) {
            seekable->seek(db->instrs[i + 1].addr);
        } else if (previous_instr && (i > 0)) {
            while (i > 0 && db->instrs[i].addr == addr) { // jump over 0 size instructions
                i--;
            }
            seekable->seek(db->instrs[i].addr);
            break;
        }
    }
}

void DisassemblerGraphView::nextInstr()
{
    seekInstruction(false);
}

void DisassemblerGraphView::prevInstr()
{
    seekInstruction(true);
}

void DisassemblerGraphView::seekLocal(RVA addr, bool update_viewport)
{
    const RVA curAddr = seekable->getOffset();
    if (addr == curAddr) {
        return;
    }
    connectSeekChanged(true);
    seekable->seek(addr);
    connectSeekChanged(false);
    if (update_viewport) {
        viewport()->update();
    }
}

void DisassemblerGraphView::copySelection()
{
    if (!highlightToken) {
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlightToken->content);
}

DisassemblerGraphView::Token *DisassemblerGraphView::getToken(Instr *instr, int x)
{
    x -= static_cast<int>(padding + charWidthA); // Ignore left margin
    if (x < 0) {
        return nullptr;
    }

    const int clickedCharPos = mFontMetrics->position(instr->plainText, x);
    if (clickedCharPos > instr->plainText.length()) {
        return nullptr;
    }

    static const QRegularExpression tokenRegExp(R"(\b(?<!\.)([^\s]+)\b(?!\.))");
    QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(instr->plainText);

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();

        if (match.capturedStart() <= clickedCharPos && match.capturedEnd() > clickedCharPos) {
            auto t = new Token;
            t->start = match.capturedStart();
            t->length = match.capturedLength();
            t->content = match.captured();
            t->instr = instr;

            return t;
        }
    }

    return nullptr;
}

QPoint DisassemblerGraphView::getInstructionOffset(const DisassemblyBlock &block, int line) const
{
    return getTextOffset(line + static_cast<int>(block.headerText.lines.size()));
}

void DisassemblerGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                         QPoint pos)
{
    Instr *instr = getInstrForMouseEvent(block, &pos, event->button() == Qt::RightButton);
    if (!instr) {
        return;
    }

    currentBlockAddress = block.entry;

    delete highlightToken;
    highlightToken = getToken(instr, pos.x());

    const RVA addr = instr->addr;
    seekLocal(addr);

    blockMenu->setOffset(addr);
    blockMenu->setCanCopy(highlightToken);
    if (highlightToken) {
        blockMenu->setCurHighlightedWord(highlightToken->content);
    }
    viewport()->update();
}

void DisassemblerGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                                      QContextMenuEvent *event, QPoint /*pos*/)
{
    const RVA offset = this->seekable->getOffset();
    actionUnhighlight.setVisible(Core()->getBBHighlighter()->getBasicBlock(block.entry));
    actionUnhighlightInstruction.setVisible(
            Core()->getBIHighlighter()->getBasicInstruction(offset));
    event->accept();
    blockMenu->exec(event->globalPos());
}

void DisassemblerGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    if (event->reason() == QContextMenuEvent::Keyboard) {
        auto *db = blockForAddress(seekable->getOffset());
        if (db) {
            auto gb = blocks[db->entry];
            blockContextMenuRequested(gb, event, QPoint());
            return;
        }
    }

    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void DisassemblerGraphView::showExportDialog()
{
    if (currentFcnAddr == RVA_INVALID) {
        qWarning() << "Cannot find current function.";
        return;
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
    showExportGraphDialog(defaultName, RZ_CORE_GRAPH_TYPE_BLOCK_FUN, currentFcnAddr);
}

void DisassemblerGraphView::blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                               QPoint pos)
{
    Q_UNUSED(event);

    const Instr *instr = getInstrForMouseEvent(block, &pos);
    if (!instr) {
        return;
    }

    DH::TargetContext ctx;
    auto bracketValue = DH::findBracketRange(instr->plainText, pos.x());

    if (bracketValue.found) {
        ctx.word = bracketValue.content;
    } else if (highlightToken) {
        ctx.word = highlightToken->content;
    } else {
        ctx.word = QString();
    }
    ctx.line = instr->plainText;
    ctx.offset = getAddrForMouseEvent(block, &pos);
    ctx.arrow = getTruePathForOffset(ctx.offset);

    const DH::TargetAction ta = DH::resolveTarget(ctx, DH::TargetFilter::Standard);
    switch (ta.type) {
    case DH::TargetType::TypeName:
        Core()->showTypeInTypesWidget(ctx.word);
        break;
    case DH::TargetType::XRefComment:
    case DH::TargetType::VariableXRef:
    case DH::TargetType::Arrow:
        if (ta.value != RVA_INVALID) {
            seekable->seek(ta.value);
        }
        break;
    case DH::TargetType::None:
        seekable->seekToReference(ctx.offset);
        break;
    default:
        break;
    }
}

void DisassemblerGraphView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event,
                                           QPoint pos)
{
    const Instr *instr = getInstrForMouseEvent(block, &pos);
    if (!instr || instr->fullText.lines.empty()) {
        QToolTip::hideText();
        event->ignore();
        return;
    }

    QToolTip::showText(event->globalPos(), instr->fullText.toQString());
}

bool DisassemblerGraphView::helpEvent(QHelpEvent *event)
{
    if (!GraphView::helpEvent(event)) {
        QToolTip::hideText();
        event->ignore();
    }

    return true;
}

void DisassemblerGraphView::blockTransitionedTo(GraphView::GraphBlock *to)
{
    currentBlockAddress = to->entry;
    if (transitionDontSeek) {
        transitionDontSeek = false;
        return;
    }
    seekLocal(to->entry);
}

void DisassemblerGraphView::onActionHighlightBITriggered()
{
    const RVA offset = this->seekable->getOffset();
    const Instr *instr = instrForAddress(offset);

    if (!instr) {
        return;
    }

    auto bih = Core()->getBIHighlighter();
    QColor background = ConfigColor("linehl");
    if (auto currentColor = bih->getBasicInstruction(offset)) {
        background = currentColor->color;
    }

    const QColor c =
            QColorDialog::getColor(background, this, QString(), QColorDialog::DontUseNativeDialog);
    if (c.isValid()) {
        bih->highlight(instr->addr, instr->size, c);
    }
    Config()->colorsUpdated();
}

void DisassemblerGraphView::onActionUnhighlightBITriggered()
{
    const RVA offset = this->seekable->getOffset();
    const Instr *instr = instrForAddress(offset);

    if (!instr) {
        return;
    }

    auto bih = Core()->getBIHighlighter();
    bih->clear(instr->addr, instr->size);
    Config()->colorsUpdated();
}

void DisassemblerGraphView::restoreCurrentBlock()
{
    onSeekChanged(this->seekable->getOffset()); // try to keep the view on current block
}

void DisassemblerGraphView::paintEvent(QPaintEvent *event)
{
    // DisassemblerGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}

bool DisassemblerGraphView::Instr::contains(ut64 addr) const
{
    return this->addr <= addr && (addr - this->addr) < size;
}
