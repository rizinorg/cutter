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

    mMenu->addSeparator();
    actionSyncOffset.setText(tr("Sync/unsync offset"));
    mMenu->addAction(&actionSyncOffset);

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

OverviewView::~OverviewView()
{
    for (QShortcut *shortcut : shortcuts) {
        delete shortcut;
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
    Q_UNUSED(from);
    Q_UNUSED(to);
    EdgeConfiguration ec;
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
