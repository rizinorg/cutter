#ifndef CALL_GRAPH_WIDGET_H
#define CALL_GRAPH_WIDGET_H

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/SimpleTextGraphView.h"

class MainWindow;

class CallGraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    CallGraphView(QWidget *parent, MainWindow *main, bool global);
    void showExportDialog() override;
protected:
    void loadCurrentGraph() override;
    bool global;
};


class CallGraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit CallGraphWidget(MainWindow *main, bool global);
    ~CallGraphWidget();

private:
    CallGraphView *graphView;
};

#endif // CALL_GRAPH_WIDGET_H
