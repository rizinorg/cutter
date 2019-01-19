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
    DisassemblerGraphView *graphView;
    OverviewWidget *overviewWidget;

private:
    void adjustOverview();
    void disableOverviewRect();
};

#endif // GRAPHWIDGET_H
