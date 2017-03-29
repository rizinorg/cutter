#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui {
class RelocsWidget;
}

class RelocsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QWidget *parent = 0);
    ~RelocsWidget();

    QTreeWidget    *relocsTreeWidget;

private slots:
    void on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::RelocsWidget *ui;

    MainWindow      *main;
};

#endif // RELOCSWIDGET_H
