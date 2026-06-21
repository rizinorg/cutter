#ifndef RESOURCESWIDGET_H
#define RESOURCESWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "common/AddressableItemModel.h"
#include "widgets/ListDockWidget.h"

class MainWindow;
class ResourcesWidget;

/**
 * @brief Source model for @ref ResourcesWidget
 */
class ResourcesModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend ResourcesWidget;

private:
    QList<ResourcesDescription> resources;

public:
    enum Columns : ut8 { INDEX = 0, NAME, VADDR, TYPE, SIZE, LANG, COMMENT, COUNT };
    explicit ResourcesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};

/**
 * @brief Dock widget with list of all resources identified within the loaded binary
 */
class ResourcesWidget : public ListDockWidget
{
    Q_OBJECT

private:
    ResourcesModel *model;
    AddressableFilterProxyModel *filterModel;

public:
    explicit ResourcesWidget(MainWindow *main);

private slots:
    void refreshResources();
};

#endif // RESOURCESWIDGET_H
