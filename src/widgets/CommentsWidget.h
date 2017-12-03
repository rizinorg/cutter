#ifndef COMMENTSWIDGET_H
#define COMMENTSWIDGET_H

#include <memory>

#include <QDockWidget>

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class CommentsWidget;
}

class CommentsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit CommentsWidget(MainWindow *main, QWidget *parent = 0);
    ~CommentsWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_nestedCmtsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toolButton_clicked();
    void on_toolButton_2_clicked();

    void on_actionHorizontal_triggered();
    void on_actionVertical_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void refreshTree();

private:
    std::unique_ptr<Ui::CommentsWidget> ui;
    MainWindow      *main;
};

#endif // COMMENTSWIDGET_H
