#include "OverviewView.h"
#include "common/CutterSeekable.h"
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
      seekable(new CutterSeekable(this))
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
    connect(this, SIGNAL(dataSet()), this, SLOT(refreshView()));
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

void OverviewView::setData(int baseWidth, int baseHeight, std::unordered_map<ut64, GraphBlock> baseBlocks)
{
    width = baseWidth;
    height = baseHeight;
    blocks = baseBlocks;
    emit dataSet();
}

void OverviewView::connectSeekChanged(bool disconn)
{
    if (disconn) {
        disconnect(seekable, &CutterSeekable::seekableSeekChanged, this,
                   &OverviewView::onSeekChanged);
    } else {
        connect(seekable, &CutterSeekable::seekableSeekChanged, this, &OverviewView::onSeekChanged);
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
    seekable->toggleSynchronization();
    if (seekable->isSynchronized()) {
        parentWidget()->setWindowTitle(windowTitle);
    } else {
        parentWidget()->setWindowTitle(windowTitle + CutterSeekable::tr(" (unsynced)"));
    }
}

void OverviewView::adjustScale()
{
    if (horizontalScrollBar()->maximum() > 0) {
        current_scale = (qreal)viewport()->width() / (qreal)(horizontalScrollBar()->maximum() + horizontalScrollBar()->pageStep());
    }
    if (verticalScrollBar()->maximum() > 0) {
        qreal h_scale = (qreal)viewport()->height() / (qreal)(verticalScrollBar()->maximum() + verticalScrollBar()->pageStep());
        if (current_scale > h_scale) {
            current_scale = h_scale;
        }
    }
    adjustSize(viewport()->size().width(), viewport()->size().height());
    eprintf("scale is %f\n",current_scale);
    viewport()->update();
}

void OverviewView::refreshView()
{
    current_scale = 1.0;
    viewport()->update();
    adjustSize(viewport()->size().width(), viewport()->size().height());
    viewport()->update();
    adjustScale();
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
    p.setBrush(QColor(0, 0, 0, 100));
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
    qreal real_width = width * current_scale;
    qreal real_height = height * current_scale;
    qreal max_right = unscrolled_render_offset_x + real_width;
    qreal max_bottom = unscrolled_render_offset_y + real_height;
    qreal rect_right = x + w;
    qreal rect_bottom = y + h;
    if (rect_right >= max_right) {
        x = real_width - w;
    }
    if (rect_bottom >= max_bottom) {
        y = real_height - h;
    }
    if (x <= 0) {
        x = 0;
    }
    if (y <= 0) {
        y = 0;
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

void OverviewView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("gui.overview.node");
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
