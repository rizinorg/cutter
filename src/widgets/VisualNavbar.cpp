#include "VisualNavbar.h"

#include "MainWindow.h"
#include "utils/TempConfig.h"

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
    cursorGraphicsItem(nullptr),
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
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fetchAndPaintData()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(updateMetadataAndPaint()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(updateMetadataAndPaint()));

    graphicsScene = new QGraphicsScene(this);

    const QBrush bg = QBrush(QColor(74, 74, 74));

    graphicsScene->setBackgroundBrush(bg);

    this->graphicsView->setAlignment(Qt::AlignLeft);
    this->graphicsView->setMinimumHeight(20);
    this->graphicsView->setMaximumHeight(20);
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

void VisualNavbar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (previousWidth != this->width()) {
        this->fillData();
        previousWidth = this->width();
    }
}

void VisualNavbar::fetchAndPaintData()
{
    fetchData();
    fillData();
}

bool VisualNavbar::sortSectionLessThan(const SectionDescription &section1,
                                       const SectionDescription &section2)
{
    return section1.vaddr < section2.vaddr;
}

VisualNavbar::MappedSegment *VisualNavbar::mappedSegmentForAddress(RVA addr)
{
    for (int i = 0; i < mappedSegments.length(); i++) {
        MappedSegment *mappedSegment = &mappedSegments[i];
        if ((mappedSegment->address_from <= addr) && (addr <= mappedSegment->address_to)) {
            return mappedSegment;
        }
    }
    return nullptr;
}

void VisualNavbar::fetchData()
{
    // TODO: This code is quite verbose but very readable. The goal is to
    //       experiment until we know what we want and then start to optimize.
    sections = Core()->getAllSections();

    // Sort sections so we don't have to filter for overlaps afterwards
    qSort(sections.begin(), sections.end(), sortSectionLessThan);

    mappedSegments.clear();
    for (SectionDescription section : sections) {
        bool segment_found = false;
        for (int i = 0; i < mappedSegments.count(); i++) {
            MappedSegment &mappedSegment = mappedSegments[i];
            // Check if the segment contains the section
            bool segment_contains_section_start = false;
            if ((mappedSegment.address_from <= section.vaddr) &&
                    (section.vaddr <= mappedSegment.address_to)) {
                segment_contains_section_start = true;
            }
            bool segment_contains_section_end = false;
            if ((mappedSegment.address_from <= section.vaddr + section.vsize) &&
                    (section.vaddr + section.vsize <= mappedSegment.address_to)) {
                segment_contains_section_end = true;
            }

            // Check if the section contains the segment
            bool section_contains_segment_start = false;
            bool section_contains_segment_end = false;
            if ((section.vaddr <= mappedSegment.address_from) &&
                    (mappedSegment.address_from <= section.vaddr + section.vsize)) {
                section_contains_segment_start = true;
            }
            if ((section.vaddr <= mappedSegment.address_to) &&
                    (mappedSegment.address_to <= section.vaddr + section.vsize)) {
                section_contains_segment_end = true;
            }

            if (segment_contains_section_start | segment_contains_section_end |
                    section_contains_segment_start | section_contains_segment_end) {
                if (section.vaddr < mappedSegment.address_from) {
                    mappedSegment.address_from = section.vaddr;
                }
                if (mappedSegment.address_to < section.vaddr + section.vsize) {
                    mappedSegment.address_to = section.vaddr + section.vsize;
                }
                segment_found = true;
                mappedSegment.sectionDescriptions.append(section);
                break;
            }
        }
        if (!segment_found) {
            MappedSegment mappedSegment;
            mappedSegment.address_from = section.vaddr;
            mappedSegment.address_to = section.vaddr + section.vsize;
            mappedSegment.sectionDescriptions.append(section);
            mappedSegments.append(mappedSegment);
        }
    }

    // If the file does not contain any sections we get the segments
    // from the memory maps instead.
    // It treats each map on its own, so overlapping maps will be shown
    // seperated.
    if (sections.count() == 0) {
        QJsonArray maps = Core()->cmdj("omj").array();
        for (QJsonValue mapValue : maps) {
            QJsonObject map = mapValue.toObject();
            MappedSegment mappedSegment;
            mappedSegment.address_from = map["from"].toVariant().toULongLong();
            mappedSegment.address_to = map["to"].toVariant().toULongLong();
            mappedSegments.append(mappedSegment);
        }
    }

    totalMappedSize = 0;
    for (auto &mappedSegment : mappedSegments) {
        totalMappedSize += mappedSegment.address_to - mappedSegment.address_from;
    }

    updateMetadata();
}

void VisualNavbar::updateMetadataAndPaint()
{
    qWarning() << "Update metadata & paint";
    updateMetadata();
    fillData();
}

void VisualNavbar::updateMetadata()
{
    for (int i = 0; i < mappedSegments.length(); i++) {
        mappedSegments[i].functions.clear();
        mappedSegments[i].symbols.clear();
        mappedSegments[i].strings.clear();
    }

    QList<FunctionDescription> functions = Core()->getAllFunctions();
    for (auto function : functions) {
        auto mappedSegment = mappedSegmentForAddress(function.offset);
        if (mappedSegment) {
            MappedSegmentMetadata metadata;
            metadata.address = function.offset;
            metadata.size = function.size;
            mappedSegment->functions.append(metadata);
        }
    }

    QList<SymbolDescription> symbols = Core()->getAllSymbols();
    for (auto symbol : symbols) {
        auto mappedSegment = mappedSegmentForAddress(symbol.vaddr);
        if (mappedSegment) {
            MappedSegmentMetadata metadata;
            metadata.address = symbol.vaddr;
            metadata.size = 1;
            mappedSegment->symbols.append(metadata);
        }
    }

    QList<StringDescription> strings = Core()->getAllStrings();
    for (auto string : strings) {
        MappedSegment *mappedSegment = mappedSegmentForAddress(string.vaddr);
        if (mappedSegment) {
            MappedSegmentMetadata metadata;
            metadata.address = string.vaddr;
            metadata.size = string.string.length();
            mappedSegment->strings.append(metadata);
        }
    }
}

void VisualNavbar::drawMetadata(QList<MappedSegmentMetadata> metadata,
                                RVA offset,
                                double x,
                                double width_per_byte,
                                double h, QColor color)
{
    for (auto s : metadata) {
        double block_x = x + ((double)(s.address - offset) * width_per_byte);
        double block_width = (double)s.size * width_per_byte;
        QGraphicsRectItem *rect = new QGraphicsRectItem(block_x, 0, block_width, h);
        rect->setPen(Qt::NoPen);
        rect->setBrush(QBrush(color));
        graphicsScene->addItem(rect);
    }
}

void VisualNavbar::fillData()
{
    graphicsScene->clear();
    cursorGraphicsItem = nullptr;
    // Do not try to draw if no sections are available.
    if (mappedSegments.length() == 0) {
        return;
    }

    int w = this->graphicsView->width();
    int h = this->graphicsView->height();

    double width_per_byte = (double)w / (double)totalMappedSize;
    xToAddress.clear();
    double current_x = 0;
    for (auto mappedSegment : mappedSegments) {
        RVA segment_size = mappedSegment.address_to - mappedSegment.address_from;
        double segment_width = (double)segment_size * width_per_byte;
        QGraphicsRectItem *rect = new QGraphicsRectItem(current_x, 0, segment_width, h);
        rect->setBrush(QBrush(Config()->getColor("gui.navbar.empty")));
        graphicsScene->addItem(rect);
        drawMetadata(mappedSegment.strings,
                     mappedSegment.address_from,
                     current_x,
                     width_per_byte,
                     h, Config()->getColor("gui.navbar.str"));
        drawMetadata(mappedSegment.symbols,
                     mappedSegment.address_from,
                     current_x,
                     width_per_byte,
                     h, Config()->getColor("gui.navbar.sym"));
        drawMetadata(mappedSegment.functions,
                     mappedSegment.address_from,
                     current_x,
                     width_per_byte,
                     h, Config()->getColor("gui.navbar.code"));

        // Keep track of where which memory segment is mapped so we are able to convert from
        // address to X coordinate and vice versa.
        struct xToAddress x2a;
        x2a.x_start = current_x;
        x2a.x_end = current_x + segment_width;
        x2a.address_from = mappedSegment.address_from;
        x2a.address_to = mappedSegment.address_to;
        xToAddress.append(x2a);

        current_x += segment_width;
    }

    // Update scene width
    graphicsScene->setSceneRect(graphicsScene->itemsBoundingRect());

    // Draw cursor
    drawCursor();
}

void VisualNavbar::drawCursor()
{
    RVA offset = Core()->getOffset();
    double cursor_x = addressToLocalX(offset);
    if (cursorGraphicsItem != nullptr) {
        graphicsScene->removeItem(cursorGraphicsItem);
        delete cursorGraphicsItem;
        cursorGraphicsItem = nullptr;
    }
    if (std::isnan(cursor_x)) {
        return;
    }
    int h = this->graphicsView->height();
    cursorGraphicsItem = new QGraphicsRectItem(cursor_x, 0, 2, h);
    cursorGraphicsItem->setPen(Qt::NoPen);
    cursorGraphicsItem->setBrush(QBrush(Config()->getColor("gui.navbar.err")));
    graphicsScene->addItem(cursorGraphicsItem);
}

void VisualNavbar::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    // Update cursor
    this->drawCursor();
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
    for (auto x2a : xToAddress) {
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
    for (auto x2a : xToAddress) {
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
    for (auto mappedSegment : mappedSegments) {
        for (auto section : mappedSegment.sectionDescriptions) {
            if ((section.vaddr <= address) && (address <= section.vaddr + section.vsize)) {
                ret.append(section.name);
            }
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
        for (auto section : sections) {
            ret += "  " + section + "\n";
        }
    }
    return ret;
}
