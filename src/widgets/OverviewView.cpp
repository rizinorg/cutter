#include "OverviewView.h"
#include "CutterSeekableWidget.h"
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

#include "Cutter.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/CachedFontMetrics.h"
#include "common/TempConfig.h"

OverviewView::OverviewView(QWidget *parent)
    : GraphView(parent),
      mFontMetrics(nullptr),
      mMenu(new DisassemblyContextMenu(this)),
      seekable(new CutterSeekableWidget(this))
{
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

    // Branch shortcuts
    QShortcut *shortcut_take_true = new QShortcut(QKeySequence(Qt::Key_T), this);
    shortcut_take_true->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_true, SIGNAL(activated()), this, SLOT(takeTrue()));
    QShortcut *shortcut_take_false = new QShortcut(QKeySequence(Qt::Key_F), this);
    shortcut_take_false->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_false, SIGNAL(activated()), this, SLOT(takeFalse()));

    //Export Graph menu
    mMenu->addSeparator();
    actionExportGraph.setText(tr("Export Graph"));
    mMenu->addAction(&actionExportGraph);
    connect(&actionExportGraph, SIGNAL(triggered(bool)), this, SLOT(on_actionExportGraph_triggered()));

    mMenu->addSeparator();
    actionSyncOffset.setText(tr("Sync/unsync offset"));
    mMenu->addAction(&actionSyncOffset);

    connect(&actionSyncOffset, SIGNAL(triggered(bool)), this, SLOT(toggleSync()));
    initFont();
    colorsUpdatedSlot();
}

void OverviewView::connectSeekChanged(bool disconn)
{
    if (disconn) {
        disconnect(seekable, &CutterSeekableWidget::seekChanged, this,
                   &OverviewView::onSeekChanged);
    } else {
        connect(seekable, &CutterSeekableWidget::seekChanged, this, &OverviewView::onSeekChanged);
    }
}

OverviewView::~OverviewView()
{
    for (QShortcut *shortcut : shortcuts) {
        delete shortcut;
    }
}

void OverviewView::toggleSync()
{
    seekable->toggleSyncWithCore();
    if (seekable->getSyncWithCore()) {
        parentWidget()->setWindowTitle(windowTitle);
    } else {
        parentWidget()->setWindowTitle(windowTitle + CutterSeekableWidget::tr(" (unsynced)"));
        seekable->setIndependentOffset(Core()->getOffset());
    }
}

void OverviewView::refreshView()
{
    initFont();
    current_scale = 1.0;
    adjustSize(viewport()->size().width(), viewport()->size().height());
    loadCurrentGraph();
    viewport()->update();
    if (horizontalScrollBar()->maximum() > 0) {
        current_scale = (double)viewport()->width() / (double)(horizontalScrollBar()->maximum() + horizontalScrollBar()->pageStep());
    }
    if (verticalScrollBar()->maximum() > 0) {
        double b = (double)viewport()->height() / (double)(verticalScrollBar()->maximum() + verticalScrollBar()->pageStep());
        if (current_scale > b) {
            current_scale = b;
        }
    }
    adjustSize(viewport()->size().width(), viewport()->size().height());
    viewport()->update();
}

void OverviewView::loadCurrentGraph()
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", true)
    .set("scr.color", COLOR_MODE_16M)
    .set("asm.bbline", false)
    .set("asm.lines", false)
    .set("asm.lines.fcn", false);

    QJsonArray functions;
    RAnalFunction *fcn = Core()->functionAt(seekable->getOffset());
    if (fcn) {
        QJsonDocument functionsDoc = Core()->cmdj("agJ " + RAddressString(fcn->addr));
        functions = functionsDoc.array();
    }

    disassembly_blocks.clear();
    blocks.clear();

    bool emptyGraph = functions.isEmpty();
    if (emptyGraph) {
        // If there's no function to print, just add a message
        if (!emptyText) {
            QVBoxLayout *layout = new QVBoxLayout(this);
            emptyText = new QLabel(this);
            emptyText->setText(tr("No function detected. Cannot display graph."));
            emptyText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            layout->addWidget(emptyText);
            layout->setAlignment(emptyText, Qt::AlignHCenter);
        }
        emptyText->setVisible(true);
    } else if (emptyText) {
        emptyText->setVisible(false);
    }
    // Refresh global "empty graph" variable so other widget know there is nothing to show here
    Core()->setGraphEmpty(emptyGraph);

    Analysis anal;
    anal.ready = true;

    QJsonValue funcRef = functions.first();
    QJsonObject func = funcRef.toObject();
    Function f;
    f.ready = true;
    f.entry = func["offset"].toVariant().toULongLong();

    windowTitle = tr("Graph Overview");
    QString funcName = func["name"].toString().trimmed();
    if (emptyGraph) {
        windowTitle += " (Empty)";
    } else if (!funcName.isEmpty()) {
        windowTitle += " (" + funcName + ")";
    }
    if (!seekable->getSyncWithCore()) {
        parentWidget()->setWindowTitle(windowTitle + CutterSeekableWidget::tr(" (unsynced)"));
    } else {
        parentWidget()->setWindowTitle(windowTitle);
    }

    RVA entry = func["offset"].toVariant().toULongLong();

    setEntry(entry);
    for (const QJsonValue &value : func["blocks"].toArray()) {
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
            gb.exits.push_back(block_fail);
        }
        if (block_jump) {
            if (block_fail) {
                db.true_path = block_jump;
            }
            gb.exits.push_back(block_jump);
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
                gb.exits.push_back(caseJump);
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

            // Skip last byte, otherwise it will overlap with next instruction
            i.size -= 1;

            QString disas;
            disas = op["text"].toString();

            QTextDocument textDoc;
            textDoc.setHtml(disas);
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
        f.blocks.push_back(db);

        addBlock(gb);
    }

    anal.functions[f.entry] = f;
    anal.status = "Ready.";
    anal.entry = f.entry;

    if (func["blocks"].toArray().size() > 0) {
        computeGraph(entry);
        viewport()->update();

        if (first_draw) {
            showBlock(blocks[entry]);
            first_draw = false;
        }
    }
}

void OverviewView::prepareGraphNode(GraphBlock &block)
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
    int extra = 4 * charWidth + 4;
    block.width = width + extra + charWidth;
    block.height = (height * charHeight) + extra;
}

void OverviewView::initFont()
{
    setFont(Config()->getFont());
    QFontMetricsF metrics(font());
    baseline = int(metrics.ascent());
    charWidth = metrics.width('X');
    charHeight = metrics.height();
    charOffset = 0;
    if (mFontMetrics)
        delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, font());
}

void OverviewView::drawBlock(QPainter &p, GraphView::GraphBlock &block)
{
    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.drawRect(block.x, block.y, block.width, block.height);

    breakpoints = Core()->getBreakpointsAddresses();

    // Render node
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    p.setPen(QColor(0, 0, 0, 0));
    if (db.terminal) {
        p.setBrush(retShadowColor);
    } else if (db.indirectcall) {
        p.setBrush(indirectcallShadowColor);
    } else {
        p.setBrush(QColor(0, 0, 0, 100));
    }

    // Node's shadow effect
    p.drawRect(block.x + 2, block.y + 2,
               block.width, block.height);
    p.setPen(QPen(graphNodeColor, 1));

    p.setBrush(disassemblyBackgroundColor);

    p.drawRect(block.x, block.y,
               block.width, block.height);

    p.setPen(Qt::red);
    p.setBrush(Qt::transparent);
}

void OverviewView::paintEvent(QPaintEvent *event)
{
    GraphView::paintEvent(event);
    if (rangeRect.width() == 0 && rangeRect.height() == 0) {
        return;
    }
    QPainter p(viewport());
    p.setPen(Qt::red);
    p.drawRect(rangeRect);
}

void OverviewView::mousePressEvent(QMouseEvent *event)
{
    if (rangeRect.contains(event->pos())) {
        mouseActive = true;
        initialDiff = QPointF(event->localPos().x() - rangeRect.x(), event->localPos().y() - rangeRect.y());
        return;
    }
    qreal w = rangeRect.width();
    qreal h = rangeRect.height();
    qreal x = event->localPos().x() - w/2;
    qreal y = event->localPos().y() - h/2;
    rangeRect = QRectF(x, y, w, h);
    viewport()->update();
    emit mouseMoved();
}

void OverviewView::mouseReleaseEvent(QMouseEvent *event)
{
    mouseActive = false;
    GraphView::mouseReleaseEvent(event);
}

void OverviewView::mouseMoveEvent(QMouseEvent *event)
{
    if (!mouseActive) {
        return;
    }
    qreal x = event->localPos().x() - initialDiff.x();
    qreal y = event->localPos().y() - initialDiff.y();
    qreal w = rangeRect.width();
    qreal h = rangeRect.height();
    int a = x + w + horizontalScrollBar()->value();
    int b = horizontalScrollBar()->value() + viewport()->width();
    int c = y + h + verticalScrollBar()->value();
    int d = verticalScrollBar()->value() + viewport()->height();
    if (a >= b) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + a - b);
        x = horizontalScrollBar()->minimum() + viewport()->width() - w;
    }
    if (c >= d) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + c - d);
        y = verticalScrollBar()->minimum() + viewport()->height() - h;
    }
    if (x <= 0) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + x);
        x = horizontalScrollBar()->minimum();
    }
    if (y <= 0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + y);
        y = verticalScrollBar()->minimum();
    }
    rangeRect = QRectF(x, y, w, h);
    viewport()->update();
    emit mouseMoved();
}

GraphView::EdgeConfiguration OverviewView::edgeConfiguration(GraphView::GraphBlock &from,
                                                                      GraphView::GraphBlock *to)
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
    ec.width_scale = current_scale;
    return ec;
}

RVA OverviewView::getAddrForMouseEvent(GraphBlock &block, QPoint *point)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = (2 * charWidth) + (db.header_text.lines.size() * charHeight);
    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = db.header_text.lines.size();
    if (mouse_row < cur_row) {
        return db.entry;
    }

    Instr *instr = getInstrForMouseEvent(block, point);
    if (instr) {
        return instr->addr;
    }

    return RVA_INVALID;
}

OverviewView::Instr *OverviewView::getInstrForMouseEvent(
    GraphView::GraphBlock &block, QPoint *point)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = (2 * charWidth) + (db.header_text.lines.size() * charHeight);
    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = db.header_text.lines.size();

    for (Instr &instr : db.instrs) {
        if (mouse_row < cur_row + (int)instr.text.lines.size()) {
            return &instr;
        }
        cur_row += instr.text.lines.size();
    }

    return nullptr;
}

void OverviewView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.border");
    disassemblySelectedBackgroundColor = ConfigColor("gui.disass_selected");
    mDisabledBreakpointColor = disassemblyBackgroundColor;
    graphNodeColor = ConfigColor("gui.border");
    backgroundColor = ConfigColor("gui.background");
    disassemblySelectionColor = ConfigColor("highlight");
    PCSelectionColor = ConfigColor("highlightPC");

    jmpColor = ConfigColor("graph.trufae");
    brtrueColor = ConfigColor("graph.true");
    brfalseColor = ConfigColor("graph.false");

    mCommentColor = ConfigColor("comment");
    initFont();
    refreshView();
}

void OverviewView::fontsUpdatedSlot()
{
    initFont();
    refreshView();
}

OverviewView::DisassemblyBlock *OverviewView::blockForAddress(RVA addr)
{
    for (auto &blockIt : disassembly_blocks) {
        DisassemblyBlock &db = blockIt.second;
        for (const Instr &i : db.instrs) {
            if (i.addr == RVA_INVALID || i.size == RVA_INVALID) {
                continue;
            }

            if ((i.addr <= addr) && (addr <= i.addr + i.size)) {
                return &db;
            }
        }
    }
    return nullptr;
}

void OverviewView::onSeekChanged(RVA addr)
{
    mMenu->setOffset(addr);
    DisassemblyBlock *db = blockForAddress(addr);
    if (db) {
        // This is a local address! We animated to it.
        transition_dont_seek = true;
        showBlock(&blocks[db->entry], true);
    } else {
        refreshView();
        db = blockForAddress(addr);
        if (db) {
            // This is a local address! We animated to it.
            transition_dont_seek = true;
            showBlock(&blocks[db->entry], true);
        }
    }
}

void OverviewView::takeTrue()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->true_path != RVA_INVALID) {
        seekable->seek(db->true_path);
    } else if (blocks[db->entry].exits.size()) {
        seekable->seek(blocks[db->entry].exits[0]);
    }
}

void OverviewView::takeFalse()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->false_path != RVA_INVALID) {
        seekable->seek(db->false_path);
    } else if (blocks[db->entry].exits.size()) {
        seekable->seek(blocks[db->entry].exits[0]);
    }
}

void OverviewView::seekInstruction(bool previous_instr)
{
    RVA addr = seekable->getOffset();
    DisassemblyBlock *db = blockForAddress(addr);
    if (!db) {
        return;
    }

    for (size_t i = 0; i < db->instrs.size(); i++) {
        Instr &instr = db->instrs[i];
        if (!((instr.addr <= addr) && (addr <= instr.addr + instr.size))) {
            continue;
        }

        // Found the instruction. Check if a next one exists
        if (!previous_instr && (i < db->instrs.size() - 1)) {
            seekable->seek(db->instrs[i + 1].addr);
        } else if (previous_instr && (i > 0)) {
            seekable->seek(db->instrs[i - 1].addr);
        }
    }
}

void OverviewView::seekLocal(RVA addr, bool update_viewport)
{
    connectSeekChanged(true);
    seekable->seek(addr);
    connectSeekChanged(false);
    if (update_viewport) {
        viewport()->update();
    }
}

OverviewView::Token *OverviewView::getToken(Instr *instr, int x)
{
    x -= (3 * charWidth); // Ignore left margin
    if (x < 0) {
        return nullptr;
    }

    int clickedCharPos = mFontMetrics->position(instr->plainText, x);
    if (clickedCharPos > instr->plainText.length()) {
        return nullptr;
    }

    static const QRegularExpression tokenRegExp("\\b(?<!\\.)([^\\s]+)\\b(?!\\.)");
    QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(instr->plainText);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        if (match.capturedStart() <= clickedCharPos && match.capturedEnd() > clickedCharPos) {
            Token *t = new Token;
            t->start = match.capturedStart();
            t->length = match.capturedLength();
            t->content = match.captured();
            t->instr = instr;

            return t;
        }
    }

    return nullptr;
}

void OverviewView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event,
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

bool OverviewView::helpEvent(QHelpEvent *event)
{
    if (!GraphView::helpEvent(event)) {
        QToolTip::hideText();
        event->ignore();
    }

    return true;
}

void OverviewView::blockTransitionedTo(GraphView::GraphBlock *to)
{
    if (transition_dont_seek) {
        transition_dont_seek = false;
        return;
    }
    seekLocal(to->entry);
}


void OverviewView::on_actionExportGraph_triggered()
{
    QStringList filters;
    filters.append(tr("Graphiz dot (*.dot)"));
    if (!QStandardPaths::findExecutable("dot").isEmpty()
            || !QStandardPaths::findExecutable("xdot").isEmpty()) {
        filters.append(tr("GIF (*.gif)"));
        filters.append(tr("PNG (*.png)"));
        filters.append(tr("JPEG (*.jpg)"));
        filters.append(tr("PostScript (*.ps)"));
        filters.append(tr("SVG (*.svg)"));
        filters.append(tr("JSON (*.json)"));
    }

    QFileDialog dialog(this, tr("Export Graph"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(filters);
    dialog.selectFile("graph");
    dialog.setDefaultSuffix("dot");
    if (!dialog.exec())
        return;
    int startIdx = dialog.selectedNameFilter().lastIndexOf("*.") + 2;
    int count = dialog.selectedNameFilter().length() - startIdx - 1;
    QString format = dialog.selectedNameFilter().mid(startIdx, count);
    QString fileName = dialog.selectedFiles()[0];
    if (format != "dot") {
        TempConfig tempConfig;
        tempConfig.set("graph.gv.format", format);
        qWarning() << Core()->cmd(QString("agfw \"%1\" @ $FB").arg(fileName));
        return;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't open file";
        return;
    }
    QTextStream fileOut(&file);
    fileOut << Core()->cmd("agfd $FB");
}
