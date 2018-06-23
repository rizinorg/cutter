#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main);
    ~GraphWidget();

private:
    DisassemblerGraphView *graphView;

};

#endif // GRAPHWIDGET_H
