#include "codegraphic.h"

#include "mainwindow.h"

#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

GraphicsBar::GraphicsBar(MainWindow *main, QWidget *parent) :
    QToolBar(main)
{
    setObjectName("codeGraphics");
    setWindowTitle("Code bar");
//    setMovable(false);
    setContentsMargins(0, 0, 0, 0);
    setStyleSheet("QToolBar { border: 0px; border-bottom: 0px; border-top: 0px; border-width: 0px;}");

    this->codeGraphic = new QGraphicsView();
    // Radare core found in:
    this->main = main;
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

void GraphicsBar::paintEvent(QPaintEvent *event) {
        QPainter painter(this);
        this->fillData();
    }

void GraphicsBar::fillData() {

    // Prepare the graph scene
    int w = this->codeGraphic->width();
    int h = this->codeGraphic->height();
    QGraphicsScene *scene = new QGraphicsScene();

    const QBrush bg = QBrush(QColor(74,74,74));

    scene->setBackgroundBrush(bg);
    this->codeGraphic->setRenderHints(0);
    this->codeGraphic->setScene(scene);
    this->codeGraphic->setRenderHints(QPainter::Antialiasing);
    this->codeGraphic->setToolTip("gap");

    // Parse JSON data
    QString jsonData = this->main->core->cmd("p-j");
    QJsonParseError *err = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());

    if (doc.isNull()) {
        qDebug() << "Invalid json in p-j command";
    }
    else if (doc.isObject()) {
        //get the jsonObject
        QJsonObject jObject = doc.object();

        //convert the json object to variantmap
        QVariantMap mainMap = jObject.toVariantMap();

        int from =  mainMap["from"].toInt();
        int to = mainMap["to"].toInt();
        int block = mainMap["blocksize"].toInt();
        int size = (to - from);
        int num = size / block;
        if (num < 1) {
            num = 1;
        }
        int graph_block = w / num;
        int counter = 0;

        for (auto i : mainMap["blocks"].toList()) {
            QMap<QString, QVariant> map = i.toMap();
            if (map.empty()) {
                // Fill empty color
                // addRect(qreal x, qreal y, qreal w, qreal h, const QPen &pen = QPen(), const QBrush &brush = QBrush())
                //scene->addRect(counter * graph_block, 0, graph_block ,h, QPen(Qt::NoPen), QBrush(QColor(252, 249, 190)));
                QGraphicsRectItem* rect = new QGraphicsRectItem(counter * graph_block, 0, graph_block ,h);
                rect->setPen(Qt::NoPen);
                rect->setBrush(QBrush(QColor(252, 249, 190)));
                rect->setToolTip("Data");
                scene->addItem(rect);
            } else {
                // Fill type of color
                //scene->addRect(counter * graph_block, 0, graph_block ,h, QPen(Qt::NoPen), QBrush(QColor(69, 104, 229)));
                QGraphicsRectItem* rect = new QGraphicsRectItem(counter * graph_block, 0, graph_block ,h);
                rect->setPen(Qt::NoPen);
                if (i.toMap()["functions"].toInt() == 0) {
                    rect->setBrush(QBrush(QColor(190, 190, 190)));
                } else {
                    rect->setBrush(QBrush(QColor(69, 104, 229)));
                }
                rect->setToolTip("Offset: 0x" + QString::number(i.toMap()["offset"].toInt(), 16) + "\nFunctions: " + QString::number( i.toMap()["functions"].toInt()) + "\nFlags: " + QString::number( i.toMap()["flags"].toInt()));
                scene->addItem(rect);
            }
            counter += 1;
        }
    }
}
