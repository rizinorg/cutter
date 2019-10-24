#ifndef RESOURCESWIDGET_H
#define RESOURCESWIDGET_H

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeView.h"
#include "common/AddressableItemModel.h"
#include "widgets/ListDockWidget.h"


class MainWindow;
class ResourcesWidget;

class ResourcesModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend ResourcesWidget;

private:
    QList<ResourcesDescription> *resources;

public:
    enum Columns { INDEX = 0, NAME, VADDR, TYPE, SIZE, LANG, COUNT };
    explicit ResourcesModel(QList<ResourcesDescription> *resources, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};

class ResourcesWidget : public ListDockWidget
{
    Q_OBJECT

private:
    ResourcesModel *model;
    AddressableFilterProxyModel *filterModel;
    CutterTreeView *view;
    QList<ResourcesDescription> resources;

public:
    explicit ResourcesWidget(MainWindow *main, QAction *action = nullptr);

private slots:
    void refreshResources();
};

#endif // RESOURCESWIDGET_H
