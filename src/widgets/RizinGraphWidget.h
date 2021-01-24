#ifndef RZ_GRAPH_WIDGET_H
#define RZ_GRAPH_WIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/SimpleTextGraphView.h"
#include "common/RefreshDeferrer.h"

class MainWindow;

namespace Ui {
class RizinGraphWidget;
}

class RizinGraphWidget;

/**
 * @brief Generic graph view for rizin graphs.
 * Not all rizin graph commands output the same kind of json. Only those that have following format
 * @code{.json}
 * { "nodes": [
 *      {
 *          "id": 0,
 *          "tittle": "node_0_tittle",
 *          "body": "".
 *          "out_nodes": [1, 2, 3]
 *      },
 *      ...
 * ]}
 * @endcode
 * Id don't have to be sequential. Simple text label is displayed containing concatenation of
 * label and body. No rizin builtin graph uses both. Duplicate edges and edges with target id
 * not present in the list of nodes are removed.
 */
class GenericRizinGraphView : public SimpleTextGraphView
{
    Q_OBJECT
public:
    GenericRizinGraphView(RizinGraphWidget *parent, MainWindow *main);
    void setGraphCommand(QString cmd);
    void refreshView() override;

protected:
    void loadCurrentGraph() override;

private:
    RefreshDeferrer refreshDeferrer;
    QString graphCommand;
    QString lastShownCommand;
};

class RizinGraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RizinGraphWidget(MainWindow *main);
    ~RizinGraphWidget();

private:
    std::unique_ptr<Ui::RizinGraphWidget> ui;
    GenericRizinGraphView *graphView;

    void typeChanged();
};

#endif // RZ_GRAPH_WIDGET_H
