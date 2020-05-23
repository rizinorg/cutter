#include "MemoryMapWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include <QShortcut>

MemoryMapModel::MemoryMapModel(QList<MemoryMapDescription> *memoryMaps, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
      memoryMaps(memoryMaps)
{
}

int MemoryMapModel::rowCount(const QModelIndex &) const
{
    return memoryMaps->count();
}

int MemoryMapModel::columnCount(const QModelIndex &) const
{
    return MemoryMapModel::ColumnCount;
}

QVariant MemoryMapModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= memoryMaps->count())
        return QVariant();

    const MemoryMapDescription &memoryMap = memoryMaps->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case AddrStartColumn:
            return RAddressString(memoryMap.addrStart);
        case AddrEndColumn:
            return RAddressString(memoryMap.addrEnd);
        case NameColumn:
            return memoryMap.name;
        case PermColumn:
            return memoryMap.permission;
        default:
            return QVariant();
        }
    case MemoryDescriptionRole:
        return QVariant::fromValue(memoryMap);
    default:
        return QVariant();
    }
}

QVariant MemoryMapModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case AddrStartColumn:
            return tr("Offset start");
        case AddrEndColumn:
            return tr("Offset end");
        case NameColumn:
            return tr("Name");
        case PermColumn:
            return tr("Permissions");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA MemoryMapModel::address(const QModelIndex &index) const
{
    const MemoryMapDescription &memoryMap = memoryMaps->at(index.row());
    return memoryMap.addrStart;
}

MemoryProxyModel::MemoryProxyModel(MemoryMapModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
}

bool MemoryProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    MemoryMapDescription item = index.data(
                                    MemoryMapModel::MemoryDescriptionRole).value<MemoryMapDescription>();
    return item.name.contains(filterRegExp());
}

bool MemoryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    MemoryMapDescription leftMemMap = left.data(
                                          MemoryMapModel::MemoryDescriptionRole).value<MemoryMapDescription>();
    MemoryMapDescription rightMemMap = right.data(
                                           MemoryMapModel::MemoryDescriptionRole).value<MemoryMapDescription>();

    switch (left.column()) {
    case MemoryMapModel::AddrStartColumn:
        return leftMemMap.addrStart < rightMemMap.addrStart;
    case MemoryMapModel::AddrEndColumn:
        return leftMemMap.addrEnd < rightMemMap.addrEnd;
    case MemoryMapModel::NameColumn:
        return leftMemMap.name < rightMemMap.name;
    case MemoryMapModel::PermColumn:
        return leftMemMap.permission < rightMemMap.permission;
    default:
        break;
    }

    return leftMemMap.addrStart < rightMemMap.addrStart;
}

MemoryMapWidget::MemoryMapWidget(MainWindow *main) :
    ListDockWidget(main, ListDockWidget::SearchBarPolicy::HideByDefault)
{
    setWindowTitle(tr("Memory Map"));
    setObjectName("MemoryMapWidget");

    memoryModel = new MemoryMapModel(&memoryMaps, this);
    memoryProxyModel = new MemoryProxyModel(memoryModel, this);
    setModels(memoryProxyModel);
    ui->treeView->sortByColumn(MemoryMapModel::AddrStartColumn, Qt::AscendingOrder);

    refreshDeferrer = createRefreshDeferrer([this]() {
        refreshMemoryMap();
    });

    connect(Core(), &CutterCore::refreshAll, this, &MemoryMapWidget::refreshMemoryMap);
    connect(Core(), &CutterCore::registersChanged, this, &MemoryMapWidget::refreshMemoryMap);

    showCount(false);
}

MemoryMapWidget::~MemoryMapWidget() = default;

void MemoryMapWidget::refreshMemoryMap()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    if (Core()->currentlyEmulating) {
        return;
    }
    memoryModel->beginResetModel();
    memoryMaps = Core()->getMemoryMap();
    memoryModel->endResetModel();

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->resizeColumnToContents(2);
}
