#ifndef FLIRT_WIDGET_H
#define FLIRT_WIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "menus/FlirtContextMenu.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;
class FlirtWidget;

namespace Ui {
class FlirtWidget;
}

class FlirtModel : public QAbstractListModel
{
    Q_OBJECT

    friend FlirtWidget;

public:
    enum Column {
        BinTypeColumn = 0,
        ArchNameColumn,
        ArchBitsColumn,
        NumModulesColumn,
        NameColumn,
        DetailsColumn,
        ColumnCount
    };
    enum Role { FlirtDescriptionRole = Qt::UserRole };

    FlirtModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<FlirtDescription> sigdb;
};

class FlirtProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FlirtProxyModel(FlirtModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class FlirtWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit FlirtWidget(MainWindow *main);
    ~FlirtWidget();

private slots:
    void refreshFlirt();
    void onSelectedItemChanged(const QModelIndex &index);
    void showItemContextMenu(const QPoint &pt);

private:
    std::unique_ptr<Ui::FlirtWidget> ui;

    FlirtModel *model;
    FlirtProxyModel *proxyModel;
    FlirtContextMenu *blockMenu;

    void setScrollMode();
};

#endif // FLIRT_WIDGET_H
