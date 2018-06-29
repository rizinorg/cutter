#ifndef VISUALNAVBAR_H
#define VISUALNAVBAR_H

#include <QToolBar>
#include <QGraphicsScene>

#include "Cutter.h"

class MainWindow;
class QGraphicsView;

class VisualNavbar : public QToolBar
{
    Q_OBJECT

    struct XToAddress {
        double x_start;
        double x_end;
        RVA address_from;
        RVA address_to;
    };

public:
    explicit VisualNavbar(MainWindow *main, QWidget *parent = nullptr);

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

    BlockStatistics    stats;
    int                statsWidth;

    QList<XToAddress> xToAddress;

    // Used to check whether the width changed. If yes we need to re-initialize the scene (slow)
    int previousWidth = -1;

    RVA localXToAddress(double x);
    double addressToLocalX(RVA address);
    QList<QString> sectionsForAddress(RVA address);
    QString toolTipForAddress(RVA address);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // VISUALNAVBAR_H
