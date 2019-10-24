
#ifndef ADDRESSABLEITEMMODEL_H
#define ADDRESSABLEITEMMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>

#include <core/CutterCommon.h>

class AddressableItemModelI
{
public:
    virtual RVA address(const QModelIndex &index) const = 0;
    /**
     * @brief Get name for item, optional.
     * @param index item intex
     * @return Item name or empty QString if item doesn't have short descriptive name.
     */
    virtual QString name(const QModelIndex &index) const { Q_UNUSED(index) return  QString(); }
    virtual QAbstractItemModel *asItemModel() = 0;
};

template <class ParentModel = QAbstractItemModel>
class AddressableItemModel : public ParentModel, public AddressableItemModelI
{
    static_assert (std::is_base_of<QAbstractItemModel, ParentModel>::value,
                   "ParentModel needs to inherit from QAbstractItemModel");
public:
    explicit AddressableItemModel(QObject *parent = nullptr) :  ParentModel(parent) {}
    virtual ~AddressableItemModel() {}
    QAbstractItemModel *asItemModel() { return this; }
};

class AddressableFilterProxyModel : public AddressableItemModel<QSortFilterProxyModel>
{
    using ParentClass = AddressableItemModel<QSortFilterProxyModel>;
public:
    AddressableFilterProxyModel(AddressableItemModelI *sourceModel, QObject *parent);

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &) const override;
    void setSourceModel(AddressableItemModelI *sourceModel);
private:
    void setSourceModel(QAbstractItemModel *sourceModel) override; // Don't use this directly
    AddressableItemModelI *addressableSourceModel;
};

#endif // ADDRESSABLEITEMMODEL_H
