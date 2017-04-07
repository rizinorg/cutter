#ifndef FUNCTIONSWIDGET_H
#define FUNCTIONSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui {
class FunctionsWidget;
}

class FunctionsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FunctionsWidget(MainWindow *main, QWidget *parent = 0);
    ~FunctionsWidget();

    QTreeWidget    *functionsTreeWidget;
    void fillFunctions();
    void refreshTree();
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

    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::FunctionsWidget *ui;

    MainWindow      *main;
};

#endif // FUNCTIONSWIDGET_H
