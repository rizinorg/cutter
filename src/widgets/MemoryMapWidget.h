#ifndef MEMORYMAPWIDGET_H
#define MEMORYMAPWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class MemoryMapWidget;

namespace Ui {
class MemoryMapWidget;
}

class MainWindow;
class QTreeWidgetItem;

/**
 * @brief Source model for @ref MemoryMapWidget
 */
class MemoryMapModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend MemoryMapWidget;

private:
    QList<MemoryMapDescription> memoryMaps;

public:
    enum Column : ut8 {
        AddrStartColumn = 0,
        AddrEndColumn,
        NameColumn,
        PermColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role : ut16 { MemoryDescriptionRole = Qt::UserRole };

    MemoryMapModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};

/**
 * @brief Proxy model for @ref MemoryMapWidget
 */
class MemoryProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    MemoryProxyModel(MemoryMapModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget for displaying list of memory segments
 */
class MemoryMapWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit MemoryMapWidget(MainWindow *main);
    ~MemoryMapWidget();

private slots:

    void refreshMemoryMap();

private:
    MemoryMapModel *memoryModel;
    MemoryProxyModel *memoryProxyModel;

    RefreshDeferrer *refreshDeferrer;
};

#endif // MEMORYMAPWIDGET_H
