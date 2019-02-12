#ifndef SDBDOCK_H
#define SDBDOCK_H

#include <memory>

#include "CutterDockWidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui {
class SdbDock;
}

class SdbDock : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SdbDock(MainWindow *main, QAction *action = nullptr);
    ~SdbDock();

private slots:
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_lockButton_clicked();
    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);

    void reload(QString _path = nullptr);

private:
    std::unique_ptr<Ui::SdbDock> ui;
    QString path;

};

#endif // SDBDOCK_H
