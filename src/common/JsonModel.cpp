#include "JsonModel.h"

#include <QIODevice>

JsonModel::JsonModel(QObject *parent) : QAbstractItemModel(parent)
{
    mRootItem = new JsonTreeItem;
    mHeaders.append("key");
    mHeaders.append("value");
}

JsonModel::~JsonModel()
{
    delete mRootItem;
}

bool JsonModel::load(QIODevice *device)
{
    return loadJson(device->readAll());
}

bool JsonModel::loadJson(const QByteArray &json)
{
    mDocument = QJsonDocument::fromJson(json);

    if (!mDocument.isNull()) {
        beginResetModel();
        delete mRootItem;
        if (mDocument.isArray()) {
            mRootItem = JsonTreeItem::load(QJsonValue(mDocument.array()));
        } else {
            mRootItem = JsonTreeItem::load(QJsonValue(mDocument.object()));
        }
        endResetModel();
        return true;
    }
    return false;
}

QVariant JsonModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid())
        return QVariant();

    JsonTreeItem *item = static_cast<JsonTreeItem *>(index.internalPointer());

    if (role == Qt::DisplayRole) {

        if (index.column() == 0)
            return QString("%1").arg(item->key());

        if (index.column() == 1)
            return QString("%1").arg(item->value());
    }

    return QVariant();
}

QVariant JsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {

        return mHeaders.value(section);
    } else
        return QVariant();
}

QModelIndex JsonModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    JsonTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = mRootItem;
    else
        parentItem = static_cast<JsonTreeItem *>(parent.internalPointer());

    JsonTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex JsonModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    JsonTreeItem *childItem = static_cast<JsonTreeItem *>(index.internalPointer());
    JsonTreeItem *parentItem = childItem->parent();

    if (parentItem == mRootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int JsonModel::rowCount(const QModelIndex &parent) const
{
    JsonTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = mRootItem;
    else
        parentItem = static_cast<JsonTreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

int JsonModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}
