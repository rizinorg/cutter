#include <stdexcept>
#include "AddressableItemModel.h"

AddressableFilterProxyModel::AddressableFilterProxyModel(AddressableItemModelI *sourceModel,
                                                         QObject *parent) :
    AddressableItemModel<QSortFilterProxyModel>(parent)
{
    setSourceModel(sourceModel);
    addressableSourceModel = sourceModel;
}

RVA AddressableFilterProxyModel::address(const QModelIndex &index) const
{
    return addressableSourceModel->address(this->mapToSource(index));
}

QString AddressableFilterProxyModel::name(const QModelIndex &index) const
{
    return addressableSourceModel->name(this->mapToSource(index));
}

void AddressableFilterProxyModel::setSourceModel(QAbstractItemModel *)
{
    throw new std::runtime_error("Not supported");
}

void AddressableFilterProxyModel::setSourceModel(AddressableItemModelI *sourceModel)
{
    ParentClass::setSourceModel(sourceModel->asItemModel());
    addressableSourceModel = sourceModel;
}
