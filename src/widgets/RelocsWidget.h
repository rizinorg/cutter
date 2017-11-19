#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <memory>

#include <QDockWidget>

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class RelocsWidget;
}

class RelocsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QWidget *parent = 0);
    ~RelocsWidget();

private slots:
    void on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillTreeWidget();

private:
    std::unique_ptr<Ui::RelocsWidget> ui;
    MainWindow      *main;

    void setScrollMode();
};

#endif // RELOCSWIDGET_H
