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

/**
 * @brief Generic graph view for r2 graphs.
 * Not all r2 graph commands output the same kind of json. Only those that have following format
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
 * label and body. No r2 builtin graph uses both. Duplicate edges and edges with target id
 * not present in the list of nodes are removed.
 */
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
    QString lastShownCommand;
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
