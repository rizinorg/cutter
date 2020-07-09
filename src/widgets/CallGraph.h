#ifndef CALL_GRAPH_WIDGET_H
#define CALL_GRAPH_WIDGET_H

#include "core/Cutter.h"
#include "AddressableDockWidget.h"
#include "widgets/SimpleTextGraphView.h"
#include "common/RefreshDeferrer.h"

class MainWindow;

class CallGraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    CallGraphView(CutterDockWidget *parent, MainWindow *main, bool global);
    void showExportDialog() override;
    void showAddress(RVA address);
    void refreshView() override;
protected:
    bool global;
    RVA address = RVA_INVALID;
    std::unordered_map<RVA, ut64> addressMapping;
    void loadCurrentGraph() override;
private:
    RefreshDeferrer refreshDeferrer;
};


class CallGraphWidget : public AddressableDockWidget
{
    Q_OBJECT

public:
    explicit CallGraphWidget(MainWindow *main, bool global);
    ~CallGraphWidget();
protected:
    QString getWindowTitle() const override;
private:
    CallGraphView *graphView;
    bool global;

    void onSeekChanged(RVA address);
};

#endif // CALL_GRAPH_WIDGET_H
