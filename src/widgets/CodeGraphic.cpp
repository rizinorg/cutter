#include "CodeGraphic.h"

#include "MainWindow.h"
#include "utils/TempConfig.h"

#include <QGraphicsView>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

GraphicsBar::GraphicsBar(MainWindow *main, QWidget *parent) :
    QToolBar(main),
    codeGraphic(new QGraphicsView),
    main(main)
{
    Q_UNUSED(parent);

    setObjectName("codeGraphics");
    setWindowTitle(tr("Code bar"));
    //    setMovable(false);
    setContentsMargins(0, 0, 0, 0);
    // If line below is used, with the dark theme the paintEvent is not called
    // and the result is wrong. Something to do with overwriting the style sheet :/
    //setStyleSheet("QToolBar { border: 0px; border-bottom: 0px; border-top: 0px; border-width: 0px;}");

    this->codeGraphic->setAlignment(Qt::AlignLeft);
    this->codeGraphic->setMinimumHeight(20);
    this->codeGraphic->setMaximumHeight(20);
    this->codeGraphic->setFrameShape(QFrame::NoFrame);

    /*
    QComboBox *addsCombo = new QComboBox();
    addsCombo->addItem("");
    addsCombo->addItem("Entry points");
    addsCombo->addItem("Marks");
    */
    addWidget(this->codeGraphic);
    //addWidget(addsCombo);

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fetchAndPaintData()));

    graphicsScene = new QGraphicsScene(this);
}

void GraphicsBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if(previousWidth != this->codeGraphic->width())
    {
        this->fillData();
        previousWidth = this->codeGraphic->width();
    }
}

void GraphicsBar::fetchAndPaintData()
{
    fetchData();
    fillData();
}

void GraphicsBar::fetchData()
{
    TempConfig tempConfig;
    tempConfig.set("search.in", QString("io.section"));
    sections = Core()->getAllSections();

    totalSectionsSize = 0;
    blockMaps.clear();
    for(SectionDescription section : sections)
    {
        QString command = "p-j @" + RAddressString(section.vaddr);
        QJsonDocument doc = Core()->cmdj(command);
        QJsonObject jObject = doc.object();
        QVariantMap mainMap = jObject.toVariantMap();
        blockMaps.append(mainMap);
        totalSectionsSize += section.size;
    }
}

void GraphicsBar::fillData()
{
    qDeleteAll(graphicsScene->items());
    cursorGraphicsItem = nullptr;
    int from = blockMaps.first()["from"].toInt();
    int to = blockMaps.first()["to"].toInt();

    // Prepare the graph scene
    int w = this->codeGraphic->width();
    int h = this->codeGraphic->height();


    const QBrush bg = QBrush(QColor(74, 74, 74));

    graphicsScene->setBackgroundBrush(bg);
    this->codeGraphic->setRenderHints(0);
    this->codeGraphic->setScene(graphicsScene);
    this->codeGraphic->setRenderHints(QPainter::Antialiasing);
    this->codeGraphic->setToolTip("gap");


    RVA current_address = Core()->getOffset();

    double width_per_byte = (double)w/(double)totalSectionsSize;
    xToAddress.clear();
    double x_start = 0.0;

    for(int i=0; i < sections.length(); i++)
    {
        SectionDescription section = sections[i];
        double width = ((double)section.size * width_per_byte);
        double x_end = x_start + width;
        double local_w = x_end - x_start;

        QVariantMap mainMap = blockMaps[i];
        from =  mainMap["from"].toInt();
        to = mainMap["to"].toInt();
        int block = mainMap["blocksize"].toInt();
        int size = (to - from);
        int num = 1;
        if (block != 0)
        {
            num = size / block;
        }

        if (num < 1)
        {
            num = 1;
        }

        int graph_block = local_w / num;
        if(graph_block == 0)
        {
            continue;
        }
        int counter = 0;

        for (auto j : mainMap["blocks"].toList())
        {
            QMap<QString, QVariant> map = j.toMap();

            // The X of where this block will start
            double block_x_start = x_start + (double)(counter * graph_block);

            // Keep X start and end as well as the start & end address in a list.
            // This is used to convert address to an X position on the bar and vice versa.
            struct xToAddress x2a;
            x2a.x_start = block_x_start;
            x2a.x_end = block_x_start + graph_block;
            x2a.address_from = map["offset"].toULongLong();
            x2a.address_to = map["offset"].toULongLong() + map["size"].toULongLong();
            xToAddress.append(x2a);

            if (map.empty())
            {
                // Fill as empty
                QGraphicsRectItem *rect = new QGraphicsRectItem(block_x_start, 0, graph_block, h);
                rect->setPen(Qt::NoPen);
                rect->setBrush(QBrush(QColor(252, 249, 190)));
                rect->setToolTip("Data");
                graphicsScene->addItem(rect);
            }
            else
            {
                // TODO: Make this compatible with theming
                QGraphicsRectItem *rect = new QGraphicsRectItem(block_x_start, 0, graph_block, h);
                rect->setPen(Qt::NoPen);
                if (map["functions"].toInt() > 0)
                {
                    rect->setBrush(QBrush(QColor(69, 104, 229)));
                }
                else if(map["symbols"].toInt() > 0)
                {
                    rect->setBrush(QBrush(QColor(229, 150, 69)));
                }
                else if(map["strings"].toInt() > 0)
                {
                    rect->setBrush(QBrush(QColor(104, 229, 69)));
                }
                else
                {
                    rect->setBrush(QBrush(QColor(190, 190, 190)));
                }
                rect->setToolTip(generateTooltip(section.name, map));
                graphicsScene->addItem(rect);
            }

            counter += 1;
        }
        x_start = x_end;
    }

    drawCursor();
}

void GraphicsBar::drawCursor()
{
    RVA offset = Core()->getOffset();
    double cursor_x = addressToLocalX(offset);
    if (cursorGraphicsItem != nullptr)
    {
        graphicsScene->removeItem(cursorGraphicsItem);
        delete cursorGraphicsItem;
    }
    if (cursor_x == nan(""))
    {
        return;
    }
    int h = this->codeGraphic->height();
    cursorGraphicsItem = new QGraphicsRectItem(cursor_x, 0, 2, h);
    cursorGraphicsItem->setPen(Qt::NoPen);
    cursorGraphicsItem->setBrush(QBrush(QColor(255, 0, 0)));
    graphicsScene->addItem(cursorGraphicsItem);
}

QString GraphicsBar::generateTooltip(QString section_name, QMap<QString, QVariant> map)
{
    QString ret = "";
    ret += "Offset:    0x" + QString::number(map["offset"].toInt(), 16) + "\n";
    ret += "Section:   " + section_name + "\n";
    ret += "Size:      " + QString::number(map["size"].toInt()) + "\n";
    ret += "Functions: " + QString::number(map["functions"].toInt()) + "\n";
    ret += "Flags:     " + QString::number(map["flags"].toInt()) + "\n";
    ret += "Comments:  " + QString::number(map["comments"].toInt()) + "\n";
    ret += "Symbols:   " + QString::number(map["symbols"].toInt()) + "\n";
    ret += "Strings:   " + QString::number(map["strings"].toInt()) + "\n";
    ret += "rwx: " + map["rwx"].toString() + "\n";

    return ret;
}

void GraphicsBar::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    // Re-paint, which will also update the cursor.
    this->drawCursor();
}

void GraphicsBar::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    // Convert the local X coordinate to an address.
    qreal x = event->localPos().x();
    RVA address = localXToAddress(x);
    if(address != RVA_INVALID)
    {
        Core()->seek(address);
    }
}

RVA GraphicsBar::localXToAddress(double x)
{
    for(auto x2a : xToAddress)
    {
        if ((x2a.x_start <= x) && (x <= x2a.x_end))
        {
            double offset = (x - x2a.x_start) / (x2a.x_end - x2a.x_start);
            double size = x2a.address_to - x2a.address_from;
            return x2a.address_from + (offset * size);
        }
    }
    return RVA_INVALID;
}

double GraphicsBar::addressToLocalX(RVA address)
{
    for(auto x2a : xToAddress)
    {
        if ((x2a.address_from <= address) && (address < x2a.address_to))
        {
            double offset = (double)(address - x2a.address_from) / (double)(x2a.address_to - x2a.address_from);
            double size = x2a.x_end - x2a.x_start;
            return x2a.x_start + (offset * size);
        }
    }
    return nan("");
}
