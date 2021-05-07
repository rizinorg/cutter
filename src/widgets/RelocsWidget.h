#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <memory>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "CutterDockWidget.h"
#include "core/Cutter.h"
#include "widgets/ListDockWidget.h"

class MainWindow;
class RelocsWidget;

class RelocsModel : public AddressableItemModel<QAbstractTableModel>
{
    Q_OBJECT

    friend RelocsWidget;

private:
    QList<RelocDescription> relocs;

public:
    enum Column { VAddrColumn = 0, TypeColumn, NameColumn, CommentColumn, ColumnCount };
    enum Role { RelocDescriptionRole = Qt::UserRole, AddressRole };

    RelocsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;

    void reload();
};

class RelocsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    RelocsProxyModel(RelocsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

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
