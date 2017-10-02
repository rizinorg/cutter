#ifndef ENTRYPOINTWIDGET_H
#define ENTRYPOINTWIDGET_H

#include "DockWidget.h"
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>
#include <memory>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class EntrypointWidget;
}

class EntrypointWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit EntrypointWidget(MainWindow *main, QWidget *parent = 0);
    ~EntrypointWidget();

    void setup() override;

    void refresh() override;

private slots:
    void on_entrypointTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    std::unique_ptr<Ui::EntrypointWidget> ui;
    MainWindow      *main;

    void fillEntrypoint();
    void setScrollMode();
};

#endif // ENTRYPOINTWIDGET_H
