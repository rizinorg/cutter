#ifndef VTABLESWIDGET_H
#define VTABLESWIDGET_H

#include <memory>

#include <QTreeView>
#include <QSortFilterProxyModel>
#include <QDockWidget>

#include "cutter.h"

namespace Ui
{
    class VTablesWidget;
}

class VTableModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    QList<VTableDescription> *vtables;

public:
    enum Columns { NAME = 0, ADDRESS, COUNT };

    VTableModel(QList<VTableDescription> *vtables, QObject* parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReload();
    void endReload();
};

class VTableSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    VTableSortFilterProxyModel(VTableModel* model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

class VTablesWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit VTablesWidget(QWidget *parent = 0);
    ~VTablesWidget();

private slots:
    void refreshVTables();

private:
    std::unique_ptr<Ui::VTablesWidget> ui;

    VTableModel *model;
    QSortFilterProxyModel *proxy;
    QList<VTableDescription> vtables;
};

#endif // VTABLESWIDGET_H
