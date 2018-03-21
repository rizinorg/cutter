#include "JsonTreeItem.h"

JsonTreeItem::JsonTreeItem(JsonTreeItem *parent)
{
    mParent = parent;
}

JsonTreeItem::~JsonTreeItem()
{
    qDeleteAll(mChilds);
}

void JsonTreeItem::appendChild(JsonTreeItem *item)
{
    mChilds.append(item);
}

JsonTreeItem *JsonTreeItem::child(int row)
{
    return mChilds.value(row);
}

JsonTreeItem *JsonTreeItem::parent()
{
    return mParent;
}

int JsonTreeItem::childCount() const
{
    return mChilds.count();
}

int JsonTreeItem::row() const
{
    if (mParent)
        return mParent->mChilds.indexOf(const_cast<JsonTreeItem *>(this));

    return 0;
}

void JsonTreeItem::setKey(const QString &key)
{
    mKey = key;
}

void JsonTreeItem::setValue(const QString &value)
{
    mValue = value;
}

void JsonTreeItem::setType(const QJsonValue::Type &type)
{
    mType = type;
}

QString JsonTreeItem::key() const
{
    return mKey;
}

QString JsonTreeItem::value() const
{
    return mValue;
}

QJsonValue::Type JsonTreeItem::type() const
{
    return mType;
}

JsonTreeItem *JsonTreeItem::load(const QJsonValue &value, JsonTreeItem *parent)
{
    JsonTreeItem *rootItem = new JsonTreeItem(parent);
    rootItem->setKey("root");

    if ( value.isObject()) {

        //Get all QJsonValue childs
        for (QString key : value.toObject().keys()) {
            QJsonValue v = value.toObject().value(key);
            JsonTreeItem *child = load(v, rootItem);
            child->setKey(key);
            child->setType(v.type());
            rootItem->appendChild(child);
        }

    } else if ( value.isArray()) {
        //Get all QJsonValue childs
        int index = 0;
        for (QJsonValue v : value.toArray()) {

            JsonTreeItem *child = load(v, rootItem);
            child->setKey(QString::number(index));
            child->setType(v.type());
            rootItem->appendChild(child);
            ++index;
        }
    } else {
        rootItem->setValue(value.toVariant().toString());
        rootItem->setType(value.type());
    }

    return rootItem;
}
