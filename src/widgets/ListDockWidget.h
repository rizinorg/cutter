#ifndef LISTDOCKWIDGET_H
#define LISTDOCKWIDGET_H

#include "CutterDockWidget.h"
#include "common/AddressableItemModel.h"
#include "menus/AddressableItemContextMenu.h"

#include <QAbstractItemModel>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidgetItem;
class CommentsWidget;

namespace Ui {
class ListDockWidget;
}

/**
 * @brief A dockable widget that displays data in a searchable tree or list format
 */
class CUTTER_EXPORT ListDockWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ListDockWidget(MainWindow *main);
    ~ListDockWidget() override;

protected:
    void setModels(AddressableFilterProxyModel *objectFilterProxyModel);

    std::unique_ptr<Ui::ListDockWidget> ui;

private:
    AddressableFilterProxyModel *objectFilterProxyModel = nullptr;
};

#endif // LISTDOCKWIDGET_H
