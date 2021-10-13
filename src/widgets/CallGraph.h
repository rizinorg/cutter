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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CallGraphView)                                                          \
        CallGraphView(const CallGraphView &v) = delete;                                            \
        CallGraphView &operator=(const CallGraphView &v) = delete;

#    define Q_DISABLE_MOVE(CallGraphView)                                                          \
        CallGraphView(CallGraphView &&v) = delete;                                                 \
        CallGraphView &operator=(CallGraphView &&v) = delete;

#    define Q_DISABLE_COPY_MOVE(CallGraphView)                                                     \
        Q_DISABLE_COPY(CallGraphView)                                                              \
        Q_DISABLE_MOVE(CallGraphView)
#endif

    Q_DISABLE_COPY_MOVE(CallGraphView)

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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CallGraphWidget)                                                        \
        CallGraphWidget(const CallGraphWidget &w) = delete;                                        \
        CallGraphWidget &operator=(const CallGraphWidget &w) = delete;

#    define Q_DISABLE_MOVE(CallGraphWidget)                                                        \
        CallGraphWidget(CallGraphWidget &&w) = delete;                                             \
        CallGraphWidget &operator=(CallGraphWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(CallGraphWidget)                                                   \
        Q_DISABLE_COPY(CallGraphWidget)                                                            \
        Q_DISABLE_MOVE(CallGraphWidget)
#endif

    Q_DISABLE_COPY_MOVE(CallGraphWidget)

public:
    explicit CallGraphWidget(MainWindow *main, bool global);
    ~CallGraphWidget() override;

protected:
    QString getWindowTitle() const override;

private:
    CallGraphView *graphView;
    bool global;

    void onSeekChanged(RVA address);
};

#endif // CALL_GRAPH_WIDGET_H
