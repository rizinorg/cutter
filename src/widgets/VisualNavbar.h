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
    void fetchStats();
    void updateGraphicsScene();
    void drawCursor();
    void on_seekChanged(RVA addr);

private:
    QGraphicsView     *graphicsView;
    QGraphicsScene    *graphicsScene;
    QGraphicsRectItem *cursorGraphicsItem;
    MainWindow        *main;

    BlockStatistics    stats;
    unsigned int       statsWidth = 0;
    unsigned int       previousWidth = 0;

    QList<XToAddress> xToAddress;

    RVA localXToAddress(double x);
    double addressToLocalX(RVA address);
    QList<QString> sectionsForAddress(RVA address);
    QString toolTipForAddress(RVA address);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // VISUALNAVBAR_H
