#ifndef SEGMENTSWIDGET_H
#define SEGMENTSWIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "widgets/ListDockWidget.h"

class QAbstractItemView;
class SegmentsWidget;

class SegmentsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SegmentsWidget;

private:
    QList<SegmentDescription> *segments;

public:
    enum Column { NameColumn = 0, SizeColumn, AddressColumn, EndAddressColumn, PermColumn, ColumnCount };
    enum Role { SegmentDescriptionRole = Qt::UserRole };

    SegmentsModel(QList<SegmentDescription> *segments, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int segment, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

class SegmentsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SegmentsProxyModel(SegmentsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class SegmentsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit SegmentsWidget(MainWindow *main);
    ~SegmentsWidget();

private slots:
    void refreshSegments();
private:
    QList<SegmentDescription> segments;
    SegmentsModel *segmentsModel;
};

#endif // SEGMENTSWIDGET_H
