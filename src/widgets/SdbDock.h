#ifndef SDBDOCK_H
#define SDBDOCK_H

#include "DockWidget.h"
#include <memory>

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class SdbDock;
}

class SdbDock : public DockWidget
{
    Q_OBJECT

public:
    explicit SdbDock(MainWindow *main, QWidget *parent = 0);
    ~SdbDock();

    void setup() override;

    void refresh() override;

private slots:
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_lockButton_clicked();

    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);

private:
    std::unique_ptr<Ui::SdbDock> ui;
    QString path;
    MainWindow      *main;

    void reload(QString path);
};

#endif // SDBDOCK_H
