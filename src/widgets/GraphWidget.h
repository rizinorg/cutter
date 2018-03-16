#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "CutterWidget.h"

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public CutterWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, QAction *action = nullptr);
    ~GraphWidget();

private:
    DisassemblerGraphView *graphView;

};

#endif // GRAPHWIDGET_H
