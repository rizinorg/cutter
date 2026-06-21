#ifndef FLIRT_WIDGET_H
#define FLIRT_WIDGET_H

#include "CutterDockWidget.h"
#include "menus/FlirtContextMenu.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;
class FlirtWidget;

namespace Ui {
class FlirtWidget;
}

/**
 * @brief Source model for @ref FlirtWidget
 */
class FlirtModel : public QAbstractListModel
{
    Q_OBJECT

    friend FlirtWidget;

public:
    enum Column : ut8 {
        BinTypeColumn = 0,
        ArchNameColumn,
        ArchBitsColumn,
        NumModulesColumn,
        NameColumn,
        DetailsColumn,
        ColumnCount
    };
    enum Role : ut16 { FlirtDescriptionRole = Qt::UserRole };

    FlirtModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QList<FlirtDescription> sigdb;
};

/**
 * @brief Proxy model for @ref FlirtWidget
 */
class FlirtProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FlirtProxyModel(FlirtModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget for browsing and applying FLIRT signatures to identify library functions
 */
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
