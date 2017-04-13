#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include "dashboard.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class RelocsWidget;
}

class RelocsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QWidget *parent = 0);
    ~RelocsWidget();

    void setup() override;

    void refresh() override;

private slots:
    void on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::RelocsWidget *ui;
    MainWindow      *main;

    void fillTreeWidget();
    void setScrollMode();
};

#endif // RELOCSWIDGET_H
