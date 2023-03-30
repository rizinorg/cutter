#include <stdexcept>
#include "AddressableItemModel.h"

#include <stdexcept>

AddressableFilterProxyModel::AddressableFilterProxyModel(AddressableItemModelI *sourceModel,
                                                         QObject *parent)
    : AddressableItemModel<QSortFilterProxyModel>(parent)
{
    setSourceModel(sourceModel);
    addressableSourceModel = sourceModel;
}

RVA AddressableFilterProxyModel::address(const QModelIndex &index) const
{
    if (!addressableSourceModel) {
        return RVA_INVALID;
    }
    return addressableSourceModel->address(this->mapToSource(index));
}

QString AddressableFilterProxyModel::name(const QModelIndex &index) const
{
    if (!addressableSourceModel) {
        return QString();
    }
    return addressableSourceModel->name(this->mapToSource(index));
}

void AddressableFilterProxyModel::setSourceModel(QAbstractItemModel *)
{
    throw new std::runtime_error("Not supported");
}

void AddressableFilterProxyModel::setSourceModel(AddressableItemModelI *sourceModel)
{
    ParentClass::setSourceModel(sourceModel ? sourceModel->asItemModel() : nullptr);
    addressableSourceModel = sourceModel;
}
