#include "CodeGraphic.h"

#include "MainWindow.h"

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
}

void GraphicsBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    this->fillData();
}

void GraphicsBar::fillData()
{

    // Prepare the graph scene
    int w = this->codeGraphic->width();
    int h = this->codeGraphic->height();
    QGraphicsScene *scene = new QGraphicsScene(this);

    const QBrush bg = QBrush(QColor(74, 74, 74));

    scene->setBackgroundBrush(bg);
    this->codeGraphic->setRenderHints(0);
    this->codeGraphic->setScene(scene);
    this->codeGraphic->setRenderHints(QPainter::Antialiasing);
    this->codeGraphic->setToolTip("gap");

    // Parse JSON data
    QString jsonData = CutterCore::getInstance()->cmd("p-j");
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());

    if (doc.isNull())
    {
        qDebug() << "Invalid json in p-j command";
    }
    else if (doc.isObject())
    {
        //get the jsonObject
        QJsonObject jObject = doc.object();

        //convert the json object to variantmap
        QVariantMap mainMap = jObject.toVariantMap();

        int from =  mainMap["from"].toInt();
        int to = mainMap["to"].toInt();
        int block = mainMap["blocksize"].toInt();
        int size = (to - from);
        int num = size / block;
        if (num < 1)
        {
            num = 1;
        }
        int graph_block = w / num;
        int counter = 0;

        for (auto i : mainMap["blocks"].toList())
        {
            QMap<QString, QVariant> map = i.toMap();
            if (map.empty())
            {
                // Fill empty color
                // addRect(qreal x, qreal y, qreal w, qreal h, const QPen &pen = QPen(), const QBrush &brush = QBrush())
                //scene->addRect(counter * graph_block, 0, graph_block ,h, QPen(Qt::NoPen), QBrush(QColor(252, 249, 190)));
                QGraphicsRectItem *rect = new QGraphicsRectItem(counter * graph_block, 0, graph_block, h);
                rect->setPen(Qt::NoPen);
                rect->setBrush(QBrush(QColor(252, 249, 190)));
                rect->setToolTip("Data");
                scene->addItem(rect);
            }
            else
            {
                // Fill type of color
                //scene->addRect(counter * graph_block, 0, graph_block ,h, QPen(Qt::NoPen), QBrush(QColor(69, 104, 229)));
                QGraphicsRectItem *rect = new QGraphicsRectItem(counter * graph_block, 0, graph_block, h);
                rect->setPen(Qt::NoPen);
                if (i.toMap()["functions"].toInt() == 0)
                {
                    rect->setBrush(QBrush(QColor(190, 190, 190)));
                }
                else
                {
                    rect->setBrush(QBrush(QColor(69, 104, 229)));
                }
                rect->setToolTip("Offset: 0x" + QString::number(i.toMap()["offset"].toInt(), 16) + "\nFunctions: " + QString::number(i.toMap()["functions"].toInt()) + "\nFlags: " + QString::number(i.toMap()["flags"].toInt()));
                scene->addItem(rect);
            }
            counter += 1;
        }
    }
}
