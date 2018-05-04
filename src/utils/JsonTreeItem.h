#ifndef JSONTREEITEM_H
#define JSONTREEITEM_H

#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QIcon>

#include "JsonModel.h"

class JsonTreeItem
{
public:
    JsonTreeItem(JsonTreeItem *parent = nullptr);
    ~JsonTreeItem();
    void appendChild(JsonTreeItem *item);
    JsonTreeItem *child(int row);
    JsonTreeItem *parent();
    int childCount() const;
    int row() const;
    void setKey(const QString &key);
    void setValue(const QString &value);
    void setType(const QJsonValue::Type &type);
    QString key() const;
    QString value() const;
    QJsonValue::Type type() const;
    static JsonTreeItem *load(const QJsonValue &value, JsonTreeItem *parent = nullptr);

private:
    QString mKey;
    QString mValue;
    QJsonValue::Type mType;
    QList<JsonTreeItem *> mChilds;
    JsonTreeItem *mParent;
};

#endif // JSONTREEITEM_H
