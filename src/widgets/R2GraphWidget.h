#ifndef R2_GRAPH_WIDGET_H
#define R2_GRAPH_WIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/SimpleTextGraphView.h"
#include "common/RefreshDeferrer.h"

class MainWindow;

namespace Ui {
class R2GraphWidget;
}

class R2GraphWidget;

class GenericR2GraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    GenericR2GraphView(R2GraphWidget *parent, MainWindow *main);
    void setGraphCommand(QString cmd);
    void refreshView() override;
protected:
    void loadCurrentGraph() override;
private:
    RefreshDeferrer refreshDeferrer;
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
