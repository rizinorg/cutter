#ifndef FUNCTIONSWIDGET_H
#define FUNCTIONSWIDGET_H

#include "dashboard.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class FunctionsWidget;
}

class FunctionsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit FunctionsWidget(MainWindow *main, QWidget *parent = 0);
    ~FunctionsWidget();

    void setup() override;

    void refresh() override;

    void fillFunctions();
    void addTooltips();

private slots:
    void on_functionsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void showFunctionsContextMenu(const QPoint &pt);

    void on_actionDisasAdd_comment_triggered();

    void on_actionFunctionsRename_triggered();

    void on_action_References_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void on_actionHorizontal_triggered();

    void on_actionVertical_triggered();

    void on_nestedFunctionsTree_itemDoubleClicked(QTreeWidgetItem *item, int column);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::FunctionsWidget *ui;
    MainWindow      *main;

    void refreshTree();
    void setScrollMode();
};

#endif // FUNCTIONSWIDGET_H
