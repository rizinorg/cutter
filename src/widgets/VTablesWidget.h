#ifndef VTABLESWIDGET_H
#define VTABLESWIDGET_H

#include <memory>

#include <QTreeView>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

namespace Ui {
class VTablesWidget;
}

class MainWindow;
class VTablesWidget;

class VTableModel : public QAbstractItemModel
{
    Q_OBJECT

    friend VTablesWidget;

private:
    QList<VTableDescription> *vtables;

public:
    enum Columns { NAME = 0, ADDRESS, COUNT };
    static const int VTableDescriptionRole = Qt::UserRole;

    VTableModel(QList<VTableDescription> *vtables, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class VTableSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    VTableSortFilterProxyModel(VTableModel *model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

class VTablesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit VTablesWidget(MainWindow *main, QAction *action = nullptr);
    ~VTablesWidget();

private slots:
    void refreshVTables();
    void on_vTableTreeView_doubleClicked(const QModelIndex &index);

private:
    std::unique_ptr<Ui::VTablesWidget> ui;

    VTableModel *model;
    QSortFilterProxyModel *proxy;
    QList<VTableDescription> vtables;
    CutterTreeWidget *tree;
};

#endif // VTABLESWIDGET_H
