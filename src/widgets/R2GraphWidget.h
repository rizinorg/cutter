#ifndef R2_GRAPH_WIDGET_H
#define R2_GRAPH_WIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "GenericR2GraphView.h"

class MainWindow;

namespace Ui {
class R2GraphWidget;
}

class R2GraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit R2GraphWidget(MainWindow *main);
    ~R2GraphWidget();

private:
    std::unique_ptr<Ui::R2GraphWidget> ui;
    GenericR2GraphView *graphView;
};

#endif // R2_GRAPH_WIDGET_H
