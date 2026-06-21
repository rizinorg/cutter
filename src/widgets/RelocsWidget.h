#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

class MainWindow;
class RelocsWidget;

/**
 * @brief Source model for @ref RelocsWidget
 */
class RelocsModel : public AddressableItemModel<QAbstractTableModel>
{
    Q_OBJECT

    friend RelocsWidget;

private:
    QList<RelocDescription> relocs;

public:
    enum Column : ut8 { VAddrColumn = 0, TypeColumn, NameColumn, CommentColumn, ColumnCount };
    enum Role : ut16 { RelocDescriptionRole = Qt::UserRole, AddressRole };

    RelocsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;

    void reload();
};

/**
 * @brief Proxy model for @ref RelocsWidget
 */
class RelocsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    RelocsProxyModel(RelocsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget that displays the list of relocations in a searchable table
 */
class RelocsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main);
    ~RelocsWidget();

private slots:
    void refreshRelocs();

private:
    RelocsModel *relocsModel;
    RelocsProxyModel *relocsProxyModel;
};

#endif // RELOCSWIDGET_H
