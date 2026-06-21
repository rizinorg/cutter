#ifndef ENTRYPOINTWIDGET_H
#define ENTRYPOINTWIDGET_H

#include "CutterDockWidget.h"

#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

#include <memory>

class MainWindow;
class QTreeWidget;

namespace Ui {
class EntrypointWidget;
}

/**
 * @brief Widget that displays all entry points
 */
class EntrypointWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit EntrypointWidget(MainWindow *main);
    ~EntrypointWidget();

private slots:
    void onEntrypointTreeWidgetItemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillEntrypoint();

private:
    std::unique_ptr<Ui::EntrypointWidget> ui;

    void setScrollMode();
};

#endif // ENTRYPOINTWIDGET_H
