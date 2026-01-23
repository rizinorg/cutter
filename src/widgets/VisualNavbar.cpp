#include "VisualNavbar.h"
#include "core/MainWindow.h"
#include "common/TempConfig.h"

#include <QGraphicsView>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QToolTip>
#include <QMouseEvent>
#include <QLayout>

#include <array>
#include <cmath>

static const int NAVBAR_HEIGHT = 15;
static const int LEGEND_HEIGHT = 25;
static const int TOTAL_HEIGHT = NAVBAR_HEIGHT + LEGEND_HEIGHT;

static const int LEGEND_BOX_SIZE = 16;
static const int LEGEND_BOX_Y_OFFSET = 7; // Relative to navbar bottom
static const int LEGEND_TEXT_X_OFFSET = 17;
static const int LEGEND_TEXT_Y_OFFSET = -5;
static const int LEGEND_ITEM_SPACING = 30;

VisualNavbar::VisualNavbar(MainWindow *main, QWidget *parent)
    : QToolBar(main),
      graphicsView(new QGraphicsView),
      seekGraphicsItem(nullptr),
      PCGraphicsItem(nullptr),
      legendItem(nullptr),
      main(main)
{
    Q_UNUSED(parent);

    blockTooltip = false;

    setObjectName("visualNavbar");
    setWindowTitle(tr("Visual navigation bar"));
    //    setMovable(false);
    setContentsMargins(0, 0, 0, 0);
    // If line below is used, with the dark theme the paintEvent is not called
    // and the result is wrong. Something to do with overwriting the style sheet :/
    // setStyleSheet("QToolBar { border: 0px; border-bottom: 0px; border-top: 0px; border-width:
    // 0px;}");

    /*
    QComboBox *addsCombo = new QComboBox();
    addsCombo->addItem("");
    addsCombo->addItem("Entry points");
    addsCombo->addItem("Marks");
    */
    addWidget(this->graphicsView);
    // addWidget(addsCombo);

    connect(Core(), &CutterCore::seekChanged, this, &VisualNavbar::on_seekChanged);
    connect(Core(), &CutterCore::registersChanged, this, &VisualNavbar::drawPCCursor);
    connect(Core(), &CutterCore::refreshAll, this, &VisualNavbar::fetchAndPaintData);
    connect(Core(), &CutterCore::functionsChanged, this, &VisualNavbar::fetchAndPaintData);
    connect(Core(), &CutterCore::flagsChanged, this, &VisualNavbar::fetchAndPaintData);
    connect(Core(), &CutterCore::globalVarsChanged, this, &VisualNavbar::fetchAndPaintData);

    graphicsScene = new QGraphicsScene(this);

    const QBrush bg = QBrush(QColor(74, 74, 74));

    graphicsScene->setBackgroundBrush(bg);

    this->graphicsView->setAlignment(Qt::AlignLeft);

    bool legendEnabled = Config()->getNavBarLegendEnabled();
    int initialHeight = legendEnabled ? TOTAL_HEIGHT : NAVBAR_HEIGHT;
    this->graphicsView->setMinimumHeight(initialHeight);
    this->graphicsView->setMaximumHeight(initialHeight);

    this->graphicsView->setFrameShape(QFrame::NoFrame);
    this->graphicsView->setRenderHints({});
    this->graphicsView->setScene(graphicsScene);
    this->graphicsView->setRenderHints(QPainter::Antialiasing);
    this->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // So the graphicsView doesn't intercept mouse events.
    this->graphicsView->setEnabled(false);
    this->graphicsView->installEventFilter(this);
    this->graphicsView->setMouseTracking(true);
    setMouseTracking(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &VisualNavbar::showLegendContextMenu);
}

unsigned int nextPow2(unsigned int n)
{
    unsigned int b = 0;
    while (n) {
        n >>= 1;
        b++;
    }
    return (1u << b);
}

void VisualNavbar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    auto w = static_cast<unsigned int>(width());
    bool fetch = false;
    if (statsWidth < w) {
        statsWidth = nextPow2(w);
        fetch = true;
    } else if (statsWidth > w * 4) {
        statsWidth = statsWidth > 0 ? statsWidth / 2 : 0;
        fetch = true;
    }

    if (fetch) {
        fetchAndPaintData();
    } else if (previousWidth != w) {
        this->previousWidth = w;
        updateGraphicsScene();
    }
}

void VisualNavbar::fetchAndPaintData()
{
    fetchStats();
    updateGraphicsScene();
}

void VisualNavbar::fetchStats()
{
    static const ut64 blocksCount = 2048;

    RzCoreLocked core(Core());
    stats.reset(nullptr);
    auto list =
            fromOwned(rz_core_get_boundaries_select(core, "search.from", "search.to", "search.in"));
    if (!list) {
        return;
    }
    RzListIter *iter;
    RzIOMap *map;
    ut64 from = UT64_MAX;
    ut64 to = 0;
    CutterRzListForeach (list.get(), iter, RzIOMap, map) {
        ut64 f = rz_itv_begin(map->itv);
        ut64 t = rz_itv_end(map->itv);
        if (f < from) {
            from = f;
        }
        if (t > to) {
            to = t;
        }
    }
    to--; // rz_core_analysis_get_stats takes inclusive ranges
    if (to < from) {
        return;
    }
    stats.reset(
            rz_core_analysis_get_stats(core, from, to, RZ_MAX(1, (to + 1 - from) / blocksCount)));
}

enum class DataType : int { Signature, Code, Data, String, Import, Symbol, Unexplored, Count };

void VisualNavbar::updateGraphicsScene()
{
    bool legendVisible = Config()->getNavBarLegendEnabled();
    graphicsScene->clear();
    xToAddress.clear();
    seekGraphicsItem = nullptr;
    PCGraphicsItem = nullptr;
    legendItem = nullptr;
    graphicsScene->setBackgroundBrush(QBrush(Config()->getColor("gui.navbar.unexplored")));

    if (!stats) {
        return;
    }

    int w = graphicsView->width();

    RVA totalSize = stats->to - stats->from + 1;
    RVA beginAddr = stats->from;

    double widthPerByte = (double)w
            / (double)(totalSize ? totalSize : pow(2.0, 64.0)); // account for overflow on 2^64
    auto xFromAddr = [widthPerByte, beginAddr](RVA addr) -> double {
        return (addr - beginAddr) * widthPerByte;
    };

    std::array<QBrush, static_cast<int>(DataType::Count)> dataTypeBrushes;
    dataTypeBrushes[static_cast<int>(DataType::Signature)] =
            QBrush(Config()->getColor("gui.navbar.signature"));
    dataTypeBrushes[static_cast<int>(DataType::Code)] =
            QBrush(Config()->getColor("gui.navbar.code"));
    dataTypeBrushes[static_cast<int>(DataType::Data)] =
            QBrush(Config()->getColor("gui.navbar.data"));
    dataTypeBrushes[static_cast<int>(DataType::String)] =
            QBrush(Config()->getColor("gui.navbar.str"));
    dataTypeBrushes[static_cast<int>(DataType::Import)] =
            QBrush(Config()->getColor("gui.navbar.import"));
    dataTypeBrushes[static_cast<int>(DataType::Symbol)] =
            QBrush(Config()->getColor("gui.navbar.sym"));
    dataTypeBrushes[static_cast<int>(DataType::Unexplored)] =
            QBrush(Config()->getColor("gui.navbar.unexplored"));

    DataType lastDataType = DataType::Unexplored;
    QGraphicsRectItem *dataItem = nullptr;
    QRectF dataItemRect(0.0, 0.0, 0.0, (double)NAVBAR_HEIGHT);
    for (size_t i = 0; i < rz_vector_len(&stats->blocks); i++) {
        RzCoreAnalysisStatsItem *block =
                reinterpret_cast<RzCoreAnalysisStatsItem *>(rz_vector_index_ptr(&stats->blocks, i));
        ut64 from = rz_core_analysis_stats_get_block_from(stats.get(), i);
        ut64 to = rz_core_analysis_stats_get_block_to(stats.get(), i) + 1;
        // Keep track of where which memory segment is mapped so we are able to convert from
        // address to X coordinate and vice versa.
        XToAddress x2a;
        x2a.x_start = xFromAddr(from);
        x2a.x_end = xFromAddr(to);
        x2a.address_from = from;
        x2a.address_to = to;
        xToAddress.append(x2a);

        DataType dataType;
        if (block->signatures) {
            dataType = DataType::Signature;
        } else if (block->imports) {
            dataType = DataType::Import;
        } else if (block->functions) {
            dataType = DataType::Code;
        } else if (block->strings) {
            dataType = DataType::String;
        } else if (block->symbols) {
            dataType = DataType::Symbol;
        } else if (block->in_functions) {
            dataType = DataType::Code;
        } else if (block->perm & RZ_PERM_RW && !(block->perm & RZ_PERM_X)) {
            dataType = DataType::Data;
        } else {
            lastDataType = DataType::Unexplored;
            continue;
        }

        if (dataType == lastDataType) {
            double r = xFromAddr(to);
            if (r > dataItemRect.right()) {
                dataItemRect.setRight(r);
                dataItem->setRect(dataItemRect);
            }
            dataItem->setRect(dataItemRect);
            continue;
        }

        dataItemRect.setX(xFromAddr(from));
        dataItemRect.setRight(xFromAddr(to));

        dataItem = new QGraphicsRectItem(dataItemRect);
        dataItem->setPen(Qt::NoPen);
        dataItem->setBrush(dataTypeBrushes[static_cast<int>(dataType)]);
        graphicsScene->addItem(dataItem);

        lastDataType = dataType;
    }

    drawSeekCursor();

    legendItem = new QGraphicsItemGroup();

    QColor themeBg = Config()->windowColorIsDark() ? palette().color(QPalette::Window)
                                                   : Config()->getColor("gui.background");

    QGraphicsRectItem *lBg = new QGraphicsRectItem(0, NAVBAR_HEIGHT, w, LEGEND_HEIGHT);
    lBg->setBrush(themeBg);
    lBg->setPen(Qt::NoPen);
    legendItem->addToGroup(lBg);

    struct LegendPart
    {
        QString name;
        QBrush brush;
    };
    QList<LegendPart> parts = {
        { tr("Signatures"), dataTypeBrushes[static_cast<int>(DataType::Signature)] },
        { tr("Code"), dataTypeBrushes[static_cast<int>(DataType::Code)] },
        { tr("Data"), dataTypeBrushes[static_cast<int>(DataType::Data)] },
        { tr("Strings"), dataTypeBrushes[static_cast<int>(DataType::String)] },
        { tr("Imports"), dataTypeBrushes[static_cast<int>(DataType::Import)] },
        { tr("Symbols"), dataTypeBrushes[static_cast<int>(DataType::Symbol)] },
        { tr("Unexplored"), dataTypeBrushes[static_cast<int>(DataType::Unexplored)] }
    };

    qreal curX = 10;
    qreal legY = NAVBAR_HEIGHT + LEGEND_BOX_Y_OFFSET;
    for (const auto &p : parts) {
        QGraphicsRectItem *r = new QGraphicsRectItem(curX, legY, LEGEND_BOX_SIZE, LEGEND_BOX_SIZE);
        r->setBrush(p.brush);
        r->setPen(QPen(Qt::black, 0.5));
        legendItem->addToGroup(r);

        QGraphicsTextItem *t = new QGraphicsTextItem(p.name);
        t->setPos(curX + LEGEND_TEXT_X_OFFSET, legY + LEGEND_TEXT_Y_OFFSET);
        legendItem->addToGroup(t);
        curX += t->boundingRect().width() + LEGEND_ITEM_SPACING;
    }

    graphicsScene->addItem(legendItem);
    legendItem->setVisible(legendVisible);
    graphicsScene->setSceneRect(0, 0, w, legendVisible ? TOTAL_HEIGHT : NAVBAR_HEIGHT);
}

void VisualNavbar::drawCursor(RVA addr, QColor color, QGraphicsRectItem *&graphicsItem)
{
    double cursor_x = addressToLocalX(addr);
    if (graphicsItem != nullptr) {
        graphicsScene->removeItem(graphicsItem);
        delete graphicsItem;
        graphicsItem = nullptr;
    }
    if (std::isnan(cursor_x)) {
        return;
    }
    // Subtract 1 so the 2px wide cursor is centered
    graphicsItem = new QGraphicsRectItem(cursor_x - 1, 0, 2, NAVBAR_HEIGHT);
    graphicsItem->setPen(Qt::NoPen);
    graphicsItem->setBrush(QBrush(color));
    graphicsScene->addItem(graphicsItem);
}

void VisualNavbar::drawPCCursor()
{
    drawCursor(Core()->getProgramCounterValue(), Config()->getColor("gui.navbar.pc"),
               PCGraphicsItem);
}

void VisualNavbar::drawSeekCursor()
{
    drawCursor(Core()->getOffset(), Config()->getColor("gui.navbar.seek"), seekGraphicsItem);
}

void VisualNavbar::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    // Update cursor
    this->drawSeekCursor();
}

bool VisualNavbar::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint scenePos = mouseEvent->pos();

        if (scenePos.y() <= NAVBAR_HEIGHT) {
            isDraggable = true;
            handleMouseAction(mouseEvent, scenePos);
            return true;
        }
        isDraggable = false;
        break;
    }
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint scenePos = mouseEvent->pos();

        if ((mouseEvent->buttons() & Qt::LeftButton && isDraggable)
            || scenePos.y() <= NAVBAR_HEIGHT) {
            handleMouseAction(mouseEvent, scenePos);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    default:
        break;
    }

    return QToolBar::eventFilter(watched, event);
}

void VisualNavbar::handleMouseAction(QMouseEvent *event, const QPoint &scenePos)
{
    if (blockTooltip) {
        return;
    }

    qreal x = scenePos.x();
    RVA address = localXToAddress(x);
    if (address != RVA_INVALID) {
        auto tooltipPos = qhelpers::mouseEventGlobalPos(event);
        blockTooltip = true; // on Haiku, the below call sometimes triggers another mouseMoveEvent,
                             // causing infinite recursion
        QToolTip::showText(tooltipPos, toolTipForAddress(address), this, this->rect());
        blockTooltip = false;
        if (event->buttons() & Qt::LeftButton) {
            event->accept();
            Core()->seek(address);
        }
    }
}

RVA VisualNavbar::localXToAddress(double x)
{
    for (const XToAddress &x2a : xToAddress) {
        if ((x2a.x_start <= x) && (x <= x2a.x_end)) {
            double offset = (x - x2a.x_start) / (x2a.x_end - x2a.x_start);
            double size = x2a.address_to - x2a.address_from;
            return x2a.address_from + (offset * size);
        }
    }
    return RVA_INVALID;
}

double VisualNavbar::addressToLocalX(RVA address)
{
    for (const XToAddress &x2a : xToAddress) {
        if ((x2a.address_from <= address) && (address < x2a.address_to)) {
            double offset = (double)(address - x2a.address_from)
                    / (double)(x2a.address_to - x2a.address_from);
            double size = x2a.x_end - x2a.x_start;
            return x2a.x_start + (offset * size);
        }
    }
    return nan("");
}

QList<QString> VisualNavbar::sectionsForAddress(RVA address)
{
    QList<QString> ret;
    QList<SectionDescription> sections = Core()->getAllSections();
    for (const SectionDescription &section : sections) {
        if (address >= section.vaddr && address < section.vaddr + section.vsize) {
            ret << section.name;
        }
    }
    return ret;
}

QString VisualNavbar::toolTipForAddress(RVA address)
{
    QString ret = tr("Address: %1").arg(RzAddressString(address));

    // Don't append sections when a debug task is in progress to avoid freezing the interface
    if (Core()->isDebugTaskInProgress()) {
        return ret;
    }

    auto sections = sectionsForAddress(address);
    if (sections.count()) {
        ret += "\n" + tr("Sections: \n");
        bool first = true;
        for (const QString &section : sections) {
            if (!first) {
                ret.append(QLatin1Char('\n'));
            } else {
                first = false;
            }
            ret += "  " + section;
        }
    }
    return ret;
}

void VisualNavbar::showLegendContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QAction *toggleLegend = menu.addAction(tr("Show Legend"));
    toggleLegend->setCheckable(true);
    toggleLegend->setChecked(Config()->getNavBarLegendEnabled());

    if (menu.exec(mapToGlobal(pos))) {
        bool checked = toggleLegend->isChecked();
        Config()->setNavBarLegendEnabled(checked);
        int h = checked ? TOTAL_HEIGHT : NAVBAR_HEIGHT;
        this->graphicsView->setMinimumHeight(h);
        this->graphicsView->setMaximumHeight(h);

        // Force the layout to realize the height change
        // Without this navbar jumps slightly down and then back up, when hiding legend
        this->graphicsView->updateGeometry();
        if (this->layout()) {
            this->layout()->activate();
        }

        updateGraphicsScene();
    }
}
