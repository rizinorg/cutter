
#include "DisassemblerGraphView.h"
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

const int DisassemblerGraphView::KEY_ZOOM_IN = Qt::Key_Plus + Qt::ControlModifier;
const int DisassemblerGraphView::KEY_ZOOM_OUT = Qt::Key_Minus + Qt::ControlModifier;
const int DisassemblerGraphView::KEY_ZOOM_RESET = Qt::Key_Equal + Qt::ControlModifier;

DisassemblerGraphView::DisassemblerGraphView(QWidget *parent, CutterSeekable *seekable,
                                             MainWindow *mainWindow, QList<QAction *> additionalMenuActions)
    : GraphView(parent),
      mFontMetrics(nullptr),
      blockMenu(new DisassemblyContextMenu(this, mainWindow)),
      contextMenu(new QMenu(this)),
      seekable(seekable),
      actionExportGraph(this),
      actionUnhighlight(this),
      actionUnhighlightInstruction(this)
{
    highlight_token = nullptr;
    auto *layout = new QVBoxLayout(this);
    // Signals that require a refresh all
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(functionRenamed(const QString &, const QString &)), this,
            SLOT(refreshView()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(instructionChanged(RVA)), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(refreshCodeViews()), this, SLOT(refreshView()));

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connectSeekChanged(false);

    // ESC for previous
    QShortcut *shortcut_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    shortcut_escape->setContext(Qt::WidgetShortcut);
    connect(shortcut_escape, SIGNAL(activated()), seekable, SLOT(seekPrev()));

    // Branch shortcuts
    QShortcut *shortcut_take_true = new QShortcut(QKeySequence(Qt::Key_T), this);
    shortcut_take_true->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_true, SIGNAL(activated()), this, SLOT(takeTrue()));
    QShortcut *shortcut_take_false = new QShortcut(QKeySequence(Qt::Key_F), this);
    shortcut_take_false->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_false, SIGNAL(activated()), this, SLOT(takeFalse()));

    // Navigation shortcuts
    QShortcut *shortcut_next_instr = new QShortcut(QKeySequence(Qt::Key_J), this);
    shortcut_next_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_next_instr, SIGNAL(activated()), this, SLOT(nextInstr()));
    QShortcut *shortcut_prev_instr = new QShortcut(QKeySequence(Qt::Key_K), this);
    shortcut_prev_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_prev_instr, SIGNAL(activated()), this, SLOT(prevInstr()));
    shortcuts.append(shortcut_escape);
    shortcuts.append(shortcut_next_instr);
    shortcuts.append(shortcut_prev_instr);

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
        , {tr("Graphviz polyline LR"), GraphView::Layout::GraphvizPolylineLR}
        , {tr("Graphviz ortho"), GraphView::Layout::GraphvizOrtho}
        , {tr("Graphviz ortho LR"), GraphView::Layout::GraphvizOrthoLR}
#endif
    };
    auto layoutMenu = contextMenu->addMenu(tr("Layout"));
    QActionGroup *layoutGroup = new QActionGroup(layoutMenu);
    for (auto &item : LAYOUT_CONFIG) {
        auto action = layoutGroup->addAction(item.first);
        action->setCheckable(true);
        GraphView::Layout layout = item.second;
        connect(action, &QAction::triggered, this, [this, layout]() {
            setGraphLayout(layout);
            refreshView();
            onSeekChanged(this->seekable->getOffset()); // try to keep the view on current block
        });
        if (layout == getGraphLayout()) {
            action->setChecked(true);
        }
    }
    layoutMenu->addActions(layoutGroup->actions());
    contextMenu->addSeparator();
    contextMenu->addActions(additionalMenuActions);


    QAction *highlightBB = new QAction(this);
    actionUnhighlight.setVisible(false);

    highlightBB->setText(tr("Highlight block"));
    connect(highlightBB, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        RVA currBlockEntry = blockForAddress(this->seekable->getOffset())->entry;

        QColor background = disassemblyBackgroundColor;
        if (auto block = bbh->getBasicBlock(currBlockEntry)) {
            background = block->color;
        }

        QColor c = QColorDialog::getColor(background, this, QString(),
                                          QColorDialog::DontUseNativeDialog);
        if (c.isValid()) {
            bbh->highlight(currBlockEntry, c);
        }
        Config()->colorsUpdated();
    });

    actionUnhighlight.setText(tr("Unhighlight block"));
    connect(&actionUnhighlight, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        bbh->clear(blockForAddress(this->seekable->getOffset())->entry);
        Config()->colorsUpdated();
    });

    QAction *highlightBI = new QAction(this);
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


    initFont();
    colorsUpdatedSlot();

    connect(blockMenu, &DisassemblyContextMenu::copy, this, &DisassemblerGraphView::copySelection);

    // Add header as widget to layout so it stretches to the layout width
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    this->scale_thickness_multiplier = true;
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
    for (QShortcut *shortcut : shortcuts) {
        delete shortcut;
    }
}

void DisassemblerGraphView::refreshView()
{
    initFont();
    loadCurrentGraph();
    viewport()->update();
    emit viewRefreshed();
}

void DisassemblerGraphView::loadCurrentGraph()
{
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
        computeGraph(entry);
    }
}

DisassemblerGraphView::EdgeConfigurationMapping DisassemblerGraphView::getEdgeConfigurations()
{
    EdgeConfigurationMapping result;
    for (auto &block : blocks) {
        for (const auto &edge : block.second.edges) {
            result[ {block.first, edge.target}] = edgeConfiguration(block.second, &blocks[edge.target], false);
        }
    }
    return result;
}

void DisassemblerGraphView::prepareGraphNode(GraphBlock &block)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];
    int width = 0;
    int height = 0;
    for (auto &line : db.header_text.lines) {
        int lw = 0;
        for (auto &part : line)
            lw += mFontMetrics->width(part.text);
        if (lw > width)
            width = lw;
        height += 1;
    }
    for (Instr &instr : db.instrs) {
        for (auto &line : instr.text.lines) {
            int lw = 0;
            for (auto &part : line)
                lw += mFontMetrics->width(part.text);
            if (lw > width)
                width = lw;
            height += 1;
        }
    }
    int extra = static_cast<int>(4 * charWidth + 4);
    block.width = static_cast<int>(width + extra + charWidth);
    block.height = (height * charHeight) + extra;
}

void DisassemblerGraphView::cleanupEdges()
{
    for (auto &blockIt : blocks) {
        auto &block = blockIt.second;
        auto outIt = block.edges.begin();
        std::unordered_set<ut64> seenEdges;
        for (auto it = block.edges.begin(), end = block.edges.end(); it != end; ++it) {
            // remove edges going  to different functions
            // and remove duplicate edges, common in switch statements
            if (blocks.find(it->target) != blocks.end() &&
                    seenEdges.find(it->target) == seenEdges.end()) {
                *outIt++ = *it;
                seenEdges.insert(it->target);
            }
        }
        block.edges.erase(outIt, block.edges.end());
    }
}

void DisassemblerGraphView::initFont()
{
    setFont(Config()->getFont());
    QFontMetricsF metrics(font());
    baseline = int(metrics.ascent());
    charWidth = metrics.width('X');
    charHeight = static_cast<int>(metrics.height());
    charOffset = 0;
    mFontMetrics.reset(new CachedFontMetrics<qreal>(font()));
}

void DisassemblerGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    const qreal padding = 2 * charWidth;

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    breakpoints = Core()->getBreakpointsAddresses();

    // Render node
    DisassemblyBlock &db = disassembly_blocks[block.entry];
    bool block_selected = false;
    RVA selected_instruction = RVA_INVALID;

    // Figure out if the current block is selected
    RVA addr = seekable->getOffset();
    RVA PCAddr = Core()->getProgramCounterValue();
    for (const Instr &instr : db.instrs) {
        if (instr.contains(addr) && interactive) {
            block_selected = true;
            selected_instruction = instr.addr;
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

    if (block_selected) {
        p.setBrush(disassemblySelectedBackgroundColor);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }

    // Draw basic block background
    p.drawRect(blockRect);
    auto bb = Core()->getBBHighlighter()->getBasicBlock(block.entry);
    if (bb) {
        QColor color(bb->color);
        p.setBrush(color);
        p.drawRect(blockRect);
    }

    const int firstInstructionY = block.y + getInstructionOffset(db, 0).y();

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    QRect screenChar = transform.mapRect(QRect(0, 0, charWidth, charHeight));

    if (screenChar.width() * qhelpers::devicePixelRatio(p.device()) < 4) {
        return;
    }

    // Highlight selected tokens
    if (interactive && highlight_token != nullptr) {
        int y = firstInstructionY;
        qreal tokenWidth = mFontMetrics->width(highlight_token->content);

        for (const Instr &instr : db.instrs) {
            int pos = -1;

            while ((pos = instr.plainText.indexOf(highlight_token->content, pos + 1)) != -1) {
                int tokenEnd = pos + highlight_token->content.length();

                if ((pos > 0 && instr.plainText[pos - 1].isLetterOrNumber())
                        || (tokenEnd < instr.plainText.length() && instr.plainText[tokenEnd].isLetterOrNumber())) {
                    continue;
                }

                qreal widthBefore = mFontMetrics->width(instr.plainText.left(pos));
                if (charWidth * 3 + widthBefore > block.width - (10 + padding)) {
                    continue;
                }

                qreal highlightWidth = tokenWidth;
                if (charWidth * 3 + widthBefore + tokenWidth >= block.width - (10 + padding)) {
                    highlightWidth = block.width - widthBefore - (10 +  2 * padding);
                }

                QColor selectionColor = ConfigColor("wordHighlight");

                p.fillRect(QRectF(block.x + charWidth * 3 + widthBefore, y, highlightWidth,
                                  charHeight), selectionColor);
            }

            y += int(instr.text.lines.size()) * charHeight;
        }
    }

    // Render node text
    auto x = block.x + padding;
    int y = block.y + getTextOffset(0).y();
    for (auto &line : db.header_text.lines) {
        RichTextPainter::paintRichText<qreal>(&p, x, y, block.width, charHeight, 0, line,
                                              mFontMetrics.get());
        y += charHeight;
    }

    auto bih = Core()->getBIHighlighter();
    for (const Instr &instr : db.instrs) {
        const QRect instrRect = QRect(static_cast<int>(block.x + charWidth), y,
                                      static_cast<int>(block.width - (10 + padding)),
                                      int(instr.text.lines.size()) * charHeight);

        QColor instrColor;
        if (Core()->isBreakpoint(breakpoints, instr.addr)) {
            instrColor = ConfigColor("gui.breakpoint_background");
        } else if (instr.addr == PCAddr) {
            instrColor = PCSelectionColor;
        } else if (auto background = bih->getBasicInstruction(instr.addr)) {
            instrColor = background->color;
        }

        if (instrColor.isValid()) {
            p.fillRect(instrRect, instrColor);
        }

        if (selected_instruction != RVA_INVALID && selected_instruction == instr.addr) {
            p.fillRect(instrRect, disassemblySelectionColor);
        }

        for (auto &line : instr.text.lines) {
            int rectSize = qRound(charWidth);
            if (rectSize % 2) {
                rectSize++;
            }
            // Assume charWidth <= charHeight
            // TODO: Breakpoint/Cip stuff
            QRectF bpRect(x - rectSize / 3.0, y + (charHeight - rectSize) / 2.0, rectSize, rectSize);
            Q_UNUSED(bpRect);

            RichTextPainter::paintRichText<qreal>(&p, x + charWidth, y,
                                                  block.width - charWidth, charHeight, 0, line,
                                                  mFontMetrics.get());
            y += charHeight;

        }
    }
}

GraphView::EdgeConfiguration DisassemblerGraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                                      GraphView::GraphBlock *to,
                                                                      bool interactive)
{
    EdgeConfiguration ec;
    DisassemblyBlock &db = disassembly_blocks[from.entry];
    if (to->entry == db.true_path) {
        ec.color = brtrueColor;
    } else if (to->entry == db.false_path) {
        ec.color = brfalseColor;
    } else {
        ec.color = jmpColor;
    }
    ec.start_arrow = false;
    ec.end_arrow = true;
    if (interactive) {
        if (from.entry == currentBlockAddress) {
            ec.width_scale = 2.0;
        } else if (to->entry == currentBlockAddress) {
            ec.width_scale = 2.0;
        }
    }
    return ec;
}


RVA DisassemblerGraphView::getAddrForMouseEvent(GraphBlock &block, QPoint *point)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = static_cast<int>(db.header_text.lines.size());
    if (mouse_row < cur_row) {
        return db.entry;
    }

    Instr *instr = getInstrForMouseEvent(block, point);
    if (instr) {
        return instr->addr;
    }

    return RVA_INVALID;
}

DisassemblerGraphView::Instr *DisassemblerGraphView::getInstrForMouseEvent(
    GraphView::GraphBlock &block, QPoint *point, bool force)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = static_cast<int>(db.header_text.lines.size());

    for (Instr &instr : db.instrs) {
        if (mouse_row < cur_row + (int)instr.text.lines.size()) {
            return &instr;
        }
        cur_row += instr.text.lines.size();
    }
    if (force && !db.instrs.empty()) {
        if (mouse_row <= 0) {
            return &db.instrs.front();
        } else {
            return &db.instrs.back();
        }
    }

    return nullptr;
}

QRectF DisassemblerGraphView::getInstrRect(GraphView::GraphBlock &block, RVA addr) const
{
    auto blockIt = disassembly_blocks.find(block.entry);
    if (blockIt == disassembly_blocks.end()) {
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
            QPointF topLeft = getInstructionOffset(db, static_cast<int>(firstLineWithAddr));
            return QRectF(topLeft, QSizeF(block.width - 4 * charWidth,
                                          charHeight * int(currentLine - firstLineWithAddr)));
        }
        currentLine += instr.text.lines.size();
    }
    return QRectF();
}

void DisassemblerGraphView::showInstruction(GraphView::GraphBlock &block, RVA addr)
{
    QRectF rect = getInstrRect(block, addr);
    rect.translate(block.x, block.y);
    showRectangle(QRect(rect.x(), rect.y(), rect.width(), rect.height()), true);
}

// Public Slots

void DisassemblerGraphView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.alt_background");
    disassemblySelectedBackgroundColor = ConfigColor("gui.disass_selected");
    mDisabledBreakpointColor = disassemblyBackgroundColor;
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    disassemblySelectionColor = ConfigColor("lineHighlight");
    PCSelectionColor = ConfigColor("highlightPC");

    jmpColor = ConfigColor("graph.trufae");
    brtrueColor = ConfigColor("graph.true");
    brfalseColor = ConfigColor("graph.false");

    mCommentColor = ConfigColor("comment");
    initFont();
    refreshView();
}

void DisassemblerGraphView::fontsUpdatedSlot()
{
    initFont();
    refreshView();
}

DisassemblerGraphView::DisassemblyBlock *DisassemblerGraphView::blockForAddress(RVA addr)
{
    for (auto &blockIt : disassembly_blocks) {
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
    DisassemblyBlock *block = blockForAddress(addr);
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
    DisassemblyBlock *db = blockForAddress(addr);
    bool switchFunction = false;
    if (!db) {
        // not in this function, try refreshing
        refreshView();
        db = blockForAddress(addr);
        switchFunction = true;
    }
    if (db) {
        // This is a local address! We animated to it.
        transition_dont_seek = true;
        showBlock(&blocks[db->entry], !switchFunction);
        showInstruction(blocks[db->entry], addr);
    }
}

void DisassemblerGraphView::zoom(QPointF mouseRelativePos, double velocity)
{
    qreal newScale = getViewScale() * std::pow(1.25, velocity);
    setZoom(mouseRelativePos, newScale);
}

void DisassemblerGraphView::setZoom(QPointF mouseRelativePos, double scale)
{
    mouseRelativePos.rx() *= size().width();
    mouseRelativePos.ry() *= size().height();
    mouseRelativePos /= getViewScale();

    auto globalMouse = mouseRelativePos + getViewOffset();
    mouseRelativePos *= getViewScale();
    qreal newScale = scale;
    newScale = std::max(newScale, 0.05);
    mouseRelativePos /= newScale;
    setViewScale(newScale);

    // Adjusting offset, so that zooming will be approaching to the cursor.
    setViewOffset(globalMouse.toPoint() - mouseRelativePos.toPoint());

    viewport()->update();
    emit viewZoomed();
}

void DisassemblerGraphView::zoomIn()
{
    zoom(QPointF(0.5, 0.5), 1);
}

void DisassemblerGraphView::zoomOut()
{
    zoom(QPointF(0.5, 0.5), -1);
}

void DisassemblerGraphView::zoomReset()
{
    setZoom(QPointF(0.5, 0.5), 1);
}

void DisassemblerGraphView::takeTrue()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->true_path != RVA_INVALID) {
        seekable->seek(db->true_path);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void DisassemblerGraphView::takeFalse()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->false_path != RVA_INVALID) {
        seekable->seek(db->false_path);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void DisassemblerGraphView::seekInstruction(bool previous_instr)
{
    RVA addr = seekable->getOffset();
    DisassemblyBlock *db = blockForAddress(addr);
    if (!db) {
        return;
    }

    for (size_t i = 0; i < db->instrs.size(); i++) {
        Instr &instr = db->instrs[i];
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
    RVA curAddr = seekable->getOffset();
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
    if (!highlight_token) return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlight_token->content);
}

DisassemblerGraphView::Token *DisassemblerGraphView::getToken(Instr *instr, int x)
{
    x -= (int) (3 * charWidth); // Ignore left margin
    if (x < 0) {
        return nullptr;
    }

    int clickedCharPos = mFontMetrics->position(instr->plainText, x);
    if (clickedCharPos > instr->plainText.length()) {
        return nullptr;
    }

    static const QRegularExpression tokenRegExp(R"(\b(?<!\.)([^\s]+)\b(?!\.))");
    QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(instr->plainText);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

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

QPoint DisassemblerGraphView::getTextOffset(int line) const
{
    int padding = static_cast<int>(2 * charWidth);
    return QPoint(padding, padding + line * charHeight);
}

QPoint DisassemblerGraphView::getInstructionOffset(const DisassemblyBlock &block, int line) const
{
    return getTextOffset(line + static_cast<int>(block.header_text.lines.size()));
}

void DisassemblerGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                         QPoint pos)
{
    Instr *instr = getInstrForMouseEvent(block, &pos, event->button() == Qt::RightButton);
    if (!instr) {
        return;
    }

    currentBlockAddress = block.entry;

    highlight_token = getToken(instr, pos.x());

    RVA addr = instr->addr;
    seekLocal(addr);

    blockMenu->setOffset(addr);
    blockMenu->setCanCopy(highlight_token);
    if (highlight_token) {
        blockMenu->setCurHighlightedWord(highlight_token->content);
    }
    viewport()->update();
}

void DisassemblerGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                                      QContextMenuEvent *event, QPoint pos)
{
    const RVA offset = this->seekable->getOffset();
    actionUnhighlight.setVisible(Core()->getBBHighlighter()->getBasicBlock(block.entry));
    actionUnhighlightInstruction.setVisible(Core()->getBIHighlighter()->getBasicInstruction(offset));
    event->accept();
    blockMenu->exec(event->globalPos());
}

void DisassemblerGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        //TODO: handle opening block menu using keyboard
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void DisassemblerGraphView::blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                               QPoint pos)
{
    Q_UNUSED(event);
    seekable->seekToReference(getAddrForMouseEvent(block, &pos));
}

void DisassemblerGraphView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event,
                                           QPoint pos)
{
    Instr *instr = getInstrForMouseEvent(block, &pos);
    if (!instr || instr->fullText.lines.empty()) {
        QToolTip::hideText();
        event->ignore();
        return;
    }

    QToolTip::showText(event->globalPos(), instr->fullText.ToQString());
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
    if (transition_dont_seek) {
        transition_dont_seek = false;
        return;
    }
    seekLocal(to->entry);
}

bool DisassemblerGraphView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key() + keyEvent->modifiers();
        if (key == KEY_ZOOM_OUT || key == KEY_ZOOM_RESET
                || key == KEY_ZOOM_IN || (key == (KEY_ZOOM_IN | Qt::ShiftModifier))) {
            event->accept();
            return true;
        }
        break;
    }
    case QEvent::KeyPress: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key() + keyEvent->modifiers();
        if (key == KEY_ZOOM_IN || (key == (KEY_ZOOM_IN | Qt::ShiftModifier))) {
            zoomIn();
            return true;
        } else if (key == KEY_ZOOM_OUT) {
            zoomOut();
            return true;
        } else if (key == KEY_ZOOM_RESET) {
            zoomReset();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return GraphView::event(event);
}


Q_DECLARE_METATYPE(DisassemblerGraphView::GraphExportType);

void DisassemblerGraphView::on_actionExportGraph_triggered()
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

    QColor c = QColorDialog::getColor(background, this, QString(),
                                      QColorDialog::DontUseNativeDialog);
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

void DisassemblerGraphView::exportGraph(QString filePath, GraphExportType type)
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
}

void DisassemblerGraphView::exportR2GraphvizGraph(QString filePath, QString type)
{
    TempConfig tempConfig;
    tempConfig.set("graph.gv.format", type);
    qWarning() << Core()->cmdRawAt(QString("agfw \"%1\"")
                                 .arg(filePath),
                                 currentFcnAddr);
}

void DisassemblerGraphView::mousePressEvent(QMouseEvent *event)
{
    GraphView::mousePressEvent(event);
    emit graphMoved();
}

void DisassemblerGraphView::mouseMoveEvent(QMouseEvent *event)
{
    GraphView::mouseMoveEvent(event);
    emit graphMoved();
}

void DisassemblerGraphView::wheelEvent(QWheelEvent *event)
{
    // when CTRL is pressed, we zoom in/out with mouse wheel
    if (Qt::ControlModifier == event->modifiers()) {
        const QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            int numSteps = numDegrees.y() / 15;

            QPointF relativeMousePos = event->pos();
            relativeMousePos.rx() /= size().width();
            relativeMousePos.ry() /= size().height();

            zoom(relativeMousePos, numSteps);
        }
        event->accept();
    } else {
        // use mouse wheel for scrolling when CTRL is not pressed
        GraphView::wheelEvent(event);
    }
    emit graphMoved();
}

void DisassemblerGraphView::resizeEvent(QResizeEvent *event)
{
    GraphView::resizeEvent(event);
    emit resized();
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
