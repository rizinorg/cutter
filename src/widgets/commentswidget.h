#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QMenu>

class MainWindow;

namespace Ui {
class CommentsWidget;
}

class CommentsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit CommentsWidget(MainWindow *main, QWidget *parent = 0);
    ~CommentsWidget();

    QTreeWidget    *commentsTreeWidget;
    QTreeWidget    *nestedCommentsTreeWidget;
    void refreshTree();

private slots:
    void on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toolButton_clicked();

    void on_toolButton_2_clicked();

    void showTitleContextMenu(const QPoint &pt);

    void on_actionHorizontal_triggered();

    void on_actionVertical_triggered();

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::CommentsWidget *ui;

    MainWindow      *main;

    QWidget         *title_bar;
};

#endif // COMMENTSWIDGET_H
