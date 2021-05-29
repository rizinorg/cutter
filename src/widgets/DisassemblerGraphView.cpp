
#include "DisassemblerGraphView.h"
#include "common/CutterSeekable.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/TempConfig.h"
#include "common/SyntaxHighlighter.h"
#include "common/BasicBlockHighlighter.h"
#include "common/BasicInstructionHighlighter.h"
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
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QClipboard>
#include <QApplication>
#include <QAction>

#include <cmath>

DisassemblerGraphView::DisassemblerGraphView(QWidget *parent, CutterSeekable *seekable,
                                             MainWindow *mainWindow,
                                             QList<QAction *> additionalMenuActions)
    : CutterGraphView(parent),
      blockMenu(new DisassemblyContextMenu(this, mainWindow)),
      contextMenu(new QMenu(this)),
      seekable(seekable),
      actionUnhighlight(this),
      actionUnhighlightInstruction(this)
{
    highlight_token = nullptr;
    auto *layout = new QVBoxLayout(this);
    // Signals that require a refresh all
    connect(Core(), &CutterCore::refreshAll, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::commentsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::functionRenamed, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::flagsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::varsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::instructionChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::breakpointsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::functionsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::asmOptionsChanged, this, &DisassemblerGraphView::refreshView);
    connect(Core(), &CutterCore::refreshCodeViews, this, &DisassemblerGraphView::refreshView);

    connectSeekChanged(false);

    // ESC for previous
    QShortcut *shortcut_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    shortcut_escape->setContext(Qt::WidgetShortcut);
    connect(shortcut_escape, &QShortcut::activated, seekable, &CutterSeekable::seekPrev);

    // Branch shortcuts
    QShortcut *shortcut_take_true = new QShortcut(QKeySequence(Qt::Key_T), this);
    shortcut_take_true->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_true, &QShortcut::activated, this, &DisassemblerGraphView::takeTrue);
    QShortcut *shortcut_take_false = new QShortcut(QKeySequence(Qt::Key_F), this);
    shortcut_take_false->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_false, &QShortcut::activated, this, &DisassemblerGraphView::takeFalse);

    // Navigation shortcuts
    QShortcut *shortcut_next_instr = new QShortcut(QKeySequence(Qt::Key_J), this);
    shortcut_next_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_next_instr, &QShortcut::activated, this, &DisassemblerGraphView::nextInstr);
    QShortcut *shortcut_prev_instr = new QShortcut(QKeySequence(Qt::Key_K), this);
    shortcut_prev_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_prev_instr, &QShortcut::activated, this, &DisassemblerGraphView::prevInstr);
    shortcuts.append(shortcut_escape);
    shortcuts.append(shortcut_next_instr);
    shortcuts.append(shortcut_prev_instr);

    // Context menu that applies to everything
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addMenu(layoutMenu);
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
    CutterGraphView::refreshView();
    loadCurrentGraph();
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
    RzAnalysisFunction *fcn = Core()->functionIn(seekable->getOffset());
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
        if (Config()->getGraphBlockEntryOffset()) {
            // QColor(0,0,0,0) is transparent
            db.header_text = Text("[" + RAddressString(db.entry) + "]", ConfigColor("offset"),
                                  QColor(0, 0, 0, 0));
        }
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
                RVA nextOffset =
                        opArray[opIndex + 1].toObject()["offset"].toVariant().toULongLong();
                i.size = nextOffset - i.addr;
            } else {
                // or to the end of the block.
                i.size = (block_entry + block_size) - i.addr;
            }

            QTextDocument textDoc;
            textDoc.setHtml(CutterCore::ansiEscapeToHtml(op["text"].toString()));

            i.plainText = textDoc.toPlainText();

            RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);
            // Colors::colorizeAssembly(richText, textDoc.toPlainText(), 0);

            bool cropped;
            int blockLength = Config()->getGraphBlockMaxChars()
                    + Core()->getConfigb("asm.bytes") * 24 + Core()->getConfigb("asm.emu") * 10;
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
    cleanupEdges(blocks);

    if (!func["blocks"].toArray().isEmpty()) {
        computeGraphPlacement();
    }
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
    int extra = static_cast<int>(2 * padding + 4);
    qreal indent = ACharWidth;
    block.width = static_cast<int>(width + extra + indent);
    block.height = (height * charHeight) + extra;
}

void DisassemblerGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

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
    QRect screenChar = transform.mapRect(QRect(0, 0, ACharWidth, charHeight));

    if (screenChar.width() < Config()->getGraphMinFontSize()) {
        return;
    }

    qreal indent = ACharWidth;

    // Highlight selected tokens
    if (interactive && highlight_token != nullptr) {
        int y = firstInstructionY;
        qreal tokenWidth = mFontMetrics->width(highlight_token->content);

        for (const Instr &instr : db.instrs) {
            int pos = -1;

            while ((pos = instr.plainText.indexOf(highlight_token->content, pos + 1)) != -1) {
                int tokenEnd = pos + highlight_token->content.length();

                if ((pos > 0 && instr.plainText[pos - 1].isLetterOrNumber())
                    || (tokenEnd < instr.plainText.length()
                        && instr.plainText[tokenEnd].isLetterOrNumber())) {
                    continue;
                }

                qreal widthBefore = mFontMetrics->width(instr.plainText.left(pos));
                qreal textOffset = padding + indent;
                if (textOffset + widthBefore > block.width - (10 + padding)) {
                    continue;
                }

                qreal highlightWidth = tokenWidth;
                if (textOffset + widthBefore + tokenWidth >= block.width - (10 + padding)) {
                    highlightWidth = block.width - widthBefore - (10 + 2 * padding);
                }

                QColor selectionColor = ConfigColor("wordHighlight");

                p.fillRect(
                        QRectF(block.x + textOffset + widthBefore, y, highlightWidth, charHeight),
                        selectionColor);
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
        const QRect instrRect = QRect(static_cast<int>(block.x + indent), y,
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

    // If mouse coordinate is in header region or margin above
    if (mouse_row < 0) {
        return db.entry;
    }

    Instr *instr = getInstrForMouseEvent(block, point);
    if (instr) {
        return instr->addr;
    }

    return RVA_INVALID;
}

DisassemblerGraphView::Instr *
DisassemblerGraphView::getInstrForMouseEvent(GraphView::GraphBlock &block, QPoint *point,
                                             bool force)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    // Row in actual text
    int cur_row = 0;

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
            return QRectF(topLeft,
                          QSizeF(block.width - 2 * padding,
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
        showBlock(blocks[db->entry], !switchFunction);
        showInstruction(blocks[db->entry], addr);
    }
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
    if (!highlight_token)
        return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlight_token->content);
}

DisassemblerGraphView::Token *DisassemblerGraphView::getToken(Instr *instr, int x)
{
    x -= (int)(padding + ACharWidth); // Ignore left margin
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
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        // TODO: handle opening block menu using keyboard
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void DisassemblerGraphView::showExportDialog()
{
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
    showExportGraphDialog(defaultName, "agf", currentFcnAddr);
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

    QColor c =
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
