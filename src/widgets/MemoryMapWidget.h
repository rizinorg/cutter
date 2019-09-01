#pragma once

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class MemoryMapWidget;

namespace Ui {
class MemoryMapWidget;
}


class MainWindow;
class QTreeWidgetItem;


class MemoryMapModel: public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend MemoryMapWidget;

private:
    QList<MemoryMapDescription> *memoryMaps;

public:
    enum Column { AddrStartColumn = 0, AddrEndColumn, NameColumn, PermColumn, ColumnCount };
    enum Role { MemoryDescriptionRole = Qt::UserRole };

    MemoryMapModel(QList<MemoryMapDescription> *memoryMaps, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};



class MemoryProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    MemoryProxyModel(MemoryMapModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class MemoryMapWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit MemoryMapWidget(MainWindow *main, QAction *action = nullptr);
    ~MemoryMapWidget();

private slots:

    void refreshMemoryMap();

private:
    MemoryMapModel *memoryModel;
    MemoryProxyModel *memoryProxyModel;
    QList<MemoryMapDescription> memoryMaps;

    RefreshDeferrer *refreshDeferrer;
};
