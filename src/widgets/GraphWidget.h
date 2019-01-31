#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class DisassemblerGraphView;
class OverviewWidget;

class GraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, OverviewWidget *overview,  QAction *action = nullptr);
    ~GraphWidget();
private:
    DisassemblerGraphView *graphView;
    OverviewWidget *overviewWidget;

    void toggleOverview(bool visibility);
    void disableOverviewRect();
private slots:
    void adjustOverview();
    void adjustGraph();
    void adjustOffset();
};

#endif // GRAPHWIDGET_H
