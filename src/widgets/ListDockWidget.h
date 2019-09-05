#ifndef LISTDOCKWIDGET_H
#define LISTDOCKWIDGET_H

#include <memory>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>

#include "core/Cutter.h"
#include "common/AddressableItemModel.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"
#include "menus/AddressableItemContextMenu.h"

class MainWindow;
class QTreeWidgetItem;
class CommentsWidget;

namespace Ui {
class ListDockWidget;
}


class ListDockWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    enum class SearchBarPolicy {
        ShowByDefault,
        HideByDefault,
        Hide,
    };

    explicit ListDockWidget(MainWindow *main, QAction *action = nullptr, SearchBarPolicy searchBarPolicy = SearchBarPolicy::ShowByDefault);
    ~ListDockWidget() override;

    void showCount(bool show);
protected:
    void setModels(AddressableFilterProxyModel *objectFilterProxyModel);

    std::unique_ptr<Ui::ListDockWidget> ui;
private:
    AddressableFilterProxyModel *objectFilterProxyModel;
    CutterTreeWidget *tree;
    SearchBarPolicy searchBarPolicy;
};

#endif // LISTDOCKWIDGET_H
