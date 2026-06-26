#ifndef VTABLESWIDGET_H
#define VTABLESWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"

#include <QSortFilterProxyModel>
#include <QTreeView>

#include <memory>

namespace Ui {
class VTablesWidget;
}

class MainWindow;
class VTablesWidget;

/**
 * @brief Source model for @ref VTablesWidget
 */
class VTableModel : public QAbstractItemModel
{
    Q_OBJECT

    friend VTablesWidget;

private:
    QList<VTableDescription> vtables;

public:
    enum Columns : ut8 { NAME = 0, ADDRESS, COUNT };
    static const int vTableDescriptionRole = Qt::UserRole;

    VTableModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

/**
 * @brief Proxy model for @ref VTablesWidget
 */
class VTableSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    VTableSortFilterProxyModel(VTableModel *model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

/**
 * @brief Widget for listing C++ Virtual Tables (vtables) and their methods
 */
class VTablesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit VTablesWidget(MainWindow *main);
    ~VTablesWidget();

private slots:
    void refreshVTables();
    void onVTableTreeViewDoubleClicked(const QModelIndex &index);

private:
    std::unique_ptr<Ui::VTablesWidget> ui;

    VTableModel *model;
    QSortFilterProxyModel *proxy;
    RefreshDeferrer *refreshDeferrer;
};

#endif // VTABLESWIDGET_H
