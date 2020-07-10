#ifndef CALL_GRAPH_WIDGET_H
#define CALL_GRAPH_WIDGET_H

#include "core/Cutter.h"
#include "AddressableDockWidget.h"
#include "widgets/SimpleTextGraphView.h"
#include "common/RefreshDeferrer.h"

class MainWindow;
/**
 * @brief Graphview displaying either global or function callgraph.
 */
class CallGraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    CallGraphView(CutterDockWidget *parent, MainWindow *main, bool global);
    void showExportDialog() override;
    void showAddress(RVA address);
    void refreshView() override;
protected:
    bool global; ///< is this a global or function callgraph
    RVA address = RVA_INVALID; ///< function address if this is not a global callgraph
    std::unordered_map<RVA, ut64> addressMapping; ///< mapping from addresses to block id
    void loadCurrentGraph() override;
    void restoreCurrentBlock() override;
private:
    RefreshDeferrer refreshDeferrer;
    RVA lastLoadedAddress = RVA_INVALID;
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
