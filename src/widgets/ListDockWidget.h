#ifndef LISTDOCKWIDGET_H
#define LISTDOCKWIDGET_H

#include <memory>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>

#include "core/Cutter.h"
#include "common/AddressableItemModel.h"
#include "CutterDockWidget.h"
#include "menus/AddressableItemContextMenu.h"

class MainWindow;
class QTreeWidgetItem;
class CommentsWidget;

namespace Ui {
class ListDockWidget;
}

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
