#ifndef VISUALNAVBAR_H
#define VISUALNAVBAR_H

#include "CutterCommon.h"
#include "RizinCpp.h"

#include <QGraphicsScene>
#include <QToolBar>

#include <rz_core.h>

class MainWindow;
class QGraphicsView;
class QGraphicsItemGroup;

/**
 * @brief Visual navigation bar at the top for quick navigation through the binary
 */
class VisualNavbar : public QToolBar
{
    Q_OBJECT

    struct XToAddress
    {
        double xStart;
        double xEnd;
        RVA addressFrom;
        RVA addressTo;
    };

public:
    explicit VisualNavbar(MainWindow *main, QWidget *parent = nullptr);

public slots:
    void paintEvent(QPaintEvent *event) override;
    void updateGraphicsScene();

private slots:
    void fetchAndPaintData();
    void fetchStats();
    void drawSeekCursor();
    void drawPCCursor();
    void drawCursor(RVA addr, QColor color, QGraphicsRectItem *&graphicsItem);
    void onSeekChanged(RVA addr);
    void showLegendContextMenu(const QPoint &pos);

private:
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;
    QGraphicsRectItem *seekGraphicsItem;
    QGraphicsRectItem *pcGraphicsItem;
    QGraphicsItemGroup *legendItem;
    MainWindow *main;

    UniquePtrC<RzCoreAnalysisStats, &rz_core_analysis_stats_free> stats;
    unsigned int statsWidth = 0;
    unsigned int previousWidth = 0;

    QList<XToAddress> xToAddress;
    bool blockTooltip;
    bool isDraggable = true;

    RVA localXToAddress(double x);
    double addressToLocalX(RVA address);
    QList<QString> sectionsForAddress(RVA address);
    QString toolTipForAddress(RVA address);

    bool eventFilter(QObject *watched, QEvent *event) override;
    void handleMouseAction(QMouseEvent *event, const QPoint &scenePos);
};

#endif // VISUALNAVBAR_H
