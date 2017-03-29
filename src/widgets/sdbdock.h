#ifndef SDBDOCK_H
#define SDBDOCK_H

#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui {
class SdbDock;
}

class SdbDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SdbDock(MainWindow *main, QWidget *parent = 0);
    ~SdbDock();

private slots:
    void on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_lockButton_clicked();

    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);

private:
    Ui::SdbDock *ui;
    void reload(QString path);
    QString path;

    MainWindow      *main;
};

#endif // SDBDOCK_H
