#include "VisualNavbar.h"

#include "MainWindow.h"
#include "common/TempConfig.h"

#include <cmath>
#include <QGraphicsView>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QToolTip>

VisualNavbar::VisualNavbar(MainWindow *main, QWidget *parent) :
    QToolBar(main),
    graphicsView(new QGraphicsView),
    seekGraphicsItem(nullptr),
    PCGraphicsItem(nullptr),
    main(main)
{
    Q_UNUSED(parent);

    setObjectName("visualNavbar");
    setWindowTitle(tr("Visual navigation bar"));
    //    setMovable(false);
    setContentsMargins(0, 0, 0, 0);
    // If line below is used, with the dark theme the paintEvent is not called
    // and the result is wrong. Something to do with overwriting the style sheet :/
    //setStyleSheet("QToolBar { border: 0px; border-bottom: 0px; border-top: 0px; border-width: 0px;}");

    /*
    QComboBox *addsCombo = new QComboBox();
    addsCombo->addItem("");
    addsCombo->addItem("Entry points");
    addsCombo->addItem("Marks");
    */
    addWidget(this->graphicsView);
    //addWidget(addsCombo);

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(registersChanged()), this, SLOT(drawPCCursor()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fetchAndPaintData()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(fetchAndPaintData()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(fetchAndPaintData()));

    graphicsScene = new QGraphicsScene(this);

    const QBrush bg = QBrush(QColor(74, 74, 74));

    graphicsScene->setBackgroundBrush(bg);

    this->graphicsView->setAlignment(Qt::AlignLeft);
    this->graphicsView->setMinimumHeight(15);
    this->graphicsView->setMaximumHeight(15);
    this->graphicsView->setFrameShape(QFrame::NoFrame);
    this->graphicsView->setRenderHints(0);
    this->graphicsView->setScene(graphicsScene);
    this->graphicsView->setRenderHints(QPainter::Antialiasing);
    this->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // So the graphicsView doesn't intercept mouse events.
    this->graphicsView->setEnabled(false);
    this->graphicsView->setMouseTracking(true);
    setMouseTracking(true);
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
    stats = Core()->getBlockStatistics(statsWidth);
}

enum class DataType : int { Empty, Code, String, Symbol, Count };

void VisualNavbar::updateGraphicsScene()
{
    graphicsScene->clear();
    xToAddress.clear();
    seekGraphicsItem = nullptr;
    PCGraphicsItem = nullptr;
    graphicsScene->setBackgroundBrush(QBrush(Config()->getColor("gui.navbar.empty")));

    if (stats.to <= stats.from) {
        return;
    }

    int w = graphicsView->width();
    int h = graphicsView->height();

    RVA totalSize = stats.to - stats.from;
    RVA beginAddr = stats.from;

    double widthPerByte = (double)w / (double)totalSize;
    auto xFromAddr = [widthPerByte, beginAddr] (RVA addr) -> double {
        return (addr - beginAddr) * widthPerByte;
    };

    std::array<QBrush, static_cast<int>(DataType::Count)> dataTypeBrushes;
    dataTypeBrushes[static_cast<int>(DataType::Code)] = QBrush(Config()->getColor("gui.navbar.code"));
    dataTypeBrushes[static_cast<int>(DataType::String)] = QBrush(Config()->getColor("gui.navbar.str"));
    dataTypeBrushes[static_cast<int>(DataType::Symbol)] = QBrush(Config()->getColor("gui.navbar.sym"));

    DataType lastDataType = DataType::Empty;
    QGraphicsRectItem *dataItem = nullptr;
    QRectF dataItemRect(0.0, 0.0, 0.0, h);
    for (const BlockDescription &block : stats.blocks) {
        // Keep track of where which memory segment is mapped so we are able to convert from
        // address to X coordinate and vice versa.
        XToAddress x2a;
        x2a.x_start = xFromAddr(block.addr);
        x2a.x_end = xFromAddr(block.addr + block.size);
        x2a.address_from = block.addr;
        x2a.address_to = block.addr + block.size;
        xToAddress.append(x2a);

        DataType dataType;
        if (block.functions > 0) {
            dataType = DataType::Code;
        } else if (block.strings > 0) {
            dataType = DataType::String;
        } else if (block.symbols > 0) {
            dataType = DataType::Symbol;
        } else if (block.inFunctions > 0) {
            dataType = DataType::Code;
        } else {
            lastDataType = DataType::Empty;
            continue;
        }

        if (dataType == lastDataType) {
            double r = xFromAddr(block.addr + block.size);
            if (r > dataItemRect.right()) {
                dataItemRect.setRight(r);
                dataItem->setRect(dataItemRect);
            }
            dataItem->setRect(dataItemRect);
            continue;
        }

        dataItemRect.setX(xFromAddr(block.addr));
        dataItemRect.setRight(xFromAddr(block.addr + block.size));

        dataItem = new QGraphicsRectItem();
        dataItem->setPen(Qt::NoPen);
        dataItem->setBrush(dataTypeBrushes[static_cast<int>(dataType)]);
        graphicsScene->addItem(dataItem);

        lastDataType = dataType;
    }

    // Update scene width
    graphicsScene->setSceneRect(0, 0, w, h);

    drawSeekCursor();
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
    int h = this->graphicsView->height();
    graphicsItem = new QGraphicsRectItem(cursor_x, 0, 2, h);
    graphicsItem->setPen(Qt::NoPen);
    graphicsItem->setBrush(QBrush(color));
    graphicsScene->addItem(graphicsItem);
}

void VisualNavbar::drawPCCursor()
{
    drawCursor(Core()->getProgramCounterValue(), Config()->getColor("gui.navbar.pc"), PCGraphicsItem);
}

void VisualNavbar::drawSeekCursor()
{
    drawCursor(Core()->getOffset(), Config()->getColor("gui.navbar.err"), seekGraphicsItem);
}

void VisualNavbar::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    // Update cursor
    this->drawSeekCursor();
}

void VisualNavbar::mousePressEvent(QMouseEvent *event)
{
    qreal x = event->localPos().x();
    RVA address = localXToAddress(x);
    if (address != RVA_INVALID) {
        QToolTip::showText(event->globalPos(), toolTipForAddress(address), this);
        if (event->buttons() & Qt::LeftButton) {
            event->accept();
            Core()->seek(address);
        }
    }
}

void VisualNavbar::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
    mousePressEvent(event);
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
            double offset = (double)(address - x2a.address_from) / (double)(x2a.address_to - x2a.address_from);
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
    QString ret = "Address: " + RAddressString(address);
    auto sections = sectionsForAddress(address);
    if (sections.count()) {
        ret += "\nSections: \n";
        bool first = true;
        for (const QString &section : sections) {
            if (!first) {
                ret += "\n";
            } else {
                first = false;
            }
            ret += "  " + section;
        }
    }
    return ret;
}
