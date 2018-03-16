#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <memory>

#include "CutterWidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class RelocsWidget;
}

class RelocsWidget : public CutterWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QAction *action = nullptr);
    ~RelocsWidget();

private slots:
    void on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillTreeWidget();

private:
    std::unique_ptr<Ui::RelocsWidget> ui;

    void setScrollMode();
};

#endif // RELOCSWIDGET_H
