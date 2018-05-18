#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "CutterSeekableWidget.h"

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public CutterSeekableWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, QAction *action = nullptr);
    ~GraphWidget();

private:
    DisassemblerGraphView *graphView;

};

#endif // GRAPHWIDGET_H
