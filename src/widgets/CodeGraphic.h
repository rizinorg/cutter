#ifndef GRAPHICSBAR_H
#define GRAPHICSBAR_H

#include <QToolBar>

#include "cutter.h"

class MainWindow;
class QGraphicsView;

class GraphicsBar : public QToolBar
{
    Q_OBJECT

    struct xToAddress {
        double x_start;
        double x_end;
        RVA address_from;
        RVA address_to;
    };

public:
    explicit GraphicsBar(MainWindow *main, QWidget *parent = 0);

public slots:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void fetchAndPaintData();
    void fetchData();
    void fillData();
    void on_seekChanged(RVA addr);

private:
    QGraphicsView   *codeGraphic;
    MainWindow      *main;
    RVA totalSectionsSize;
    QList<SectionDescription> sections;
    QList<QVariantMap> blockMaps;
    QList<struct xToAddress> xToAddress;
    QString generateTooltip(QString section_name, QMap<QString, QVariant> map);

    void mousePressEvent(QMouseEvent *event) override;
};

#endif // GRAPHICSBAR_H
