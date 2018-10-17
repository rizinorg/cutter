
#ifndef JSONMODEL_H
#define JSONMODEL_H

#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QIcon>

#include "JsonTreeItem.h"

class JsonTreeItem;

class JsonModel : public QAbstractItemModel
{

public:
    explicit JsonModel(QObject *parent = nullptr);
    bool load(QIODevice *device);
    bool loadJson(const QByteArray &json);
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    ~JsonModel();
private:
    JsonTreeItem *mRootItem;
    QJsonDocument mDocument;
    QStringList mHeaders;
};

#endif // JSONMODEL_H
