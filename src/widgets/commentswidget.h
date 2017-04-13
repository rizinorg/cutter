#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include "dockwidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class CommentsWidget;
}

class CommentsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit CommentsWidget(MainWindow *main, QWidget *parent = 0);
    ~CommentsWidget();

    void setup() override;

    void refresh() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toolButton_clicked();

    void on_toolButton_2_clicked();

    void showTitleContextMenu(const QPoint &pt);

    void on_actionHorizontal_triggered();

    void on_actionVertical_triggered();

private:
    Ui::CommentsWidget *ui;
    MainWindow      *main;

    void refreshTree();
};

#endif // COMMENTSWIDGET_H
