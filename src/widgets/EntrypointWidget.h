#ifndef ENTRYPOINTWIDGET_H
#define ENTRYPOINTWIDGET_H

#include <memory>

#include <QDockWidget>
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class EntrypointWidget;
}

class EntrypointWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit EntrypointWidget(MainWindow *main, QWidget *parent = 0);
    ~EntrypointWidget();

private slots:
    void on_entrypointTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillEntrypoint();

private:
    std::unique_ptr<Ui::EntrypointWidget> ui;
    MainWindow      *main;

    void setScrollMode();
};

#endif // ENTRYPOINTWIDGET_H
