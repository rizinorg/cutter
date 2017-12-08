#ifndef VISUALNAVBAR_H
#define VISUALNAVBAR_H

#include <QToolBar>
#include <QGraphicsScene>

#include "cutter.h"

class MainWindow;
class QGraphicsView;

class VisualNavbar : public QToolBar
{
    Q_OBJECT

    struct xToAddress {
        double x_start;
        double x_end;
        RVA address_from;
        RVA address_to;
    };

public:
    explicit VisualNavbar(MainWindow *main, QWidget *parent = 0);

public slots:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void fetchAndPaintData();
    void fetchData();
    void fillData();
    void drawCursor();
    void on_seekChanged(RVA addr);

private:
    QGraphicsView     *graphicsView;
    QGraphicsScene    *graphicsScene;
    QGraphicsRectItem *cursorGraphicsItem;
    MainWindow        *main;
    RVA totalSectionsSize;
    QList<SectionDescription> sections;
    QList<QVariantMap> blockMaps;
    QList<struct xToAddress> xToAddress;

    // Used to check whether the width changed. If yes we need to re-initialize the scene (slow)
    int previousWidth;

    QString generateTooltip(QString section_name, QMap<QString, QVariant> map);

    RVA localXToAddress(double x);
    double addressToLocalX(RVA address);

    void mousePressEvent(QMouseEvent *event) override;
};

#endif // VISUALNAVBAR_H
