#ifndef R2_GRAPH_WIDGET_H
#define R2_GRAPH_WIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/SimpleTextGraphView.h"

class MainWindow;

namespace Ui {
class R2GraphWidget;
}

class GenericR2GraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    using SimpleTextGraphView::SimpleTextGraphView;
    void setGraphCommand(QString cmd);
protected:
    void loadCurrentGraph() override;
private:
    QString graphCommand;
};


class R2GraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit R2GraphWidget(MainWindow *main);
    ~R2GraphWidget();

private:
    std::unique_ptr<Ui::R2GraphWidget> ui;
    GenericR2GraphView *graphView;

    void typeChanged();
};

#endif // R2_GRAPH_WIDGET_H
